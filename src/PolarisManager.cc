#include "PolarisManager.h"

namespace polaris {

#define RETRY_MAX	2

class polaris_manager
{
public:
	polaris_manager(const std::string& polaris_url,
					PolarisConfig config,
					std::atomic<int> *ref);
	~polaris_manager();

	int watch_service(const std::string& service_namespace,
					  const std::string& service_name);
	int unwatch_service(const std::string& service_namespace,
						const std::string& service_name);

	int register_service(const std::string& service_namespace,
						 const std::string& service_name,
						 const PolarisInstance& instance);
	int deregister_service(const std::string& service_namespace,
						   const std::string& service_name,
						   const PolarisInstance& instance);

	void get_watching_list(std::vector<std::string>& list);

private:
	std::atomic<int> *ref;
	std::string polaris_url;
	PolarisConfig config;
	PolarisClient client;
	int retry_max;

	struct watch_info
	{
		bool watching;
		std::string service_revision;
		std::string routing_revision;
		std::condition_variable cond;
	};
	std::mutex mutex;
	std::unordered_map<std::string, struct watch_info> watch_status;
	std::unordered_map<std::string, PolarisPolicy *> unwatch_policies;
	int status;

	std::function<void (PolarisTask *task)> discover_cb;
	std::function<void (WFTimerTask *task)> timer_cb;

private:
	void discover_callback(PolarisTask *task);
	void register_callback(PolarisTask *task);
	void deregister_callback(PolarisTask *task);
	void timer_callback(WFTimerTask *task);
};

struct series_context
{
	std::string service_namespace;
	std::string service_name;
};

struct watch_result
{
	WFFacilities::WaitGroup *wait_group;
	int error;
};

PolarisManager::PolarisManager(const std::string& polaris_url)
{
	PolarisConfig config;
	this->ref = new std::atomic<int>(1);
	this->ptr = new polaris_manager(polaris_url, std::move(config), this->ref);
}

PolarisManager::PolarisManager(const std::string& polaris_url,
							   PolarisConfig config)
{
	this->ref = new std::atomic<int>(1);
	this->ptr = new polaris_manager(polaris_url, std::move(config), this->ref);
}

PolarisManager::~PolarisManager()
{
	if (--*this->ref == 0)
	{
		delete this->ptr;
		delete this->ref;
	}
}

int PolarisManager::watch_service(const std::string& service_namespace,
								  const std::string& service_name)
{
	return this->ptr->watch_service(service_namespace, service_name);
}

int PolarisManager::unwatch_service(const std::string& service_namespace,
									const std::string& service_name)
{
	return this->ptr->unwatch_service(service_namespace, service_name);
}

int PolarisManager::register_service(const std::string& service_namespace,
									 const std::string& service_name,
									 const PolarisInstance& instance)
{
	return this->ptr->register_service(service_namespace, service_name,
									   instance);
}

int PolarisManager::deregister_service(const std::string& service_namespace,
									   const std::string& service_name,
									   const PolarisInstance& instance)
{
	return this->ptr->deregister_service(service_namespace, service_name,
										 instance);
}

void PolarisManager::get_watching_list(std::vector<std::string>& list)
{
	this->ptr->get_watching_list(list);
}

polaris_manager::polaris_manager(const std::string& polaris_url,
								 PolarisConfig config,
								 std::atomic<int> *ref) :
	ref(ref),
	polaris_url(polaris_url),
	config(std::move(config))
{
	if (client.init(polaris_url) == 0)
		this->status = WFP_INIT_SUCCESS;
	else
		this->status = WFP_INIT_FAILED;

	this->retry_max = RETRY_MAX;

	this->discover_cb = std::bind(&polaris_manager::discover_callback,
								  this,
								  std::placeholders::_1);
	this->timer_cb = std::bind(&polaris_manager::timer_callback,
							   this,
							   std::placeholders::_1);
}

polaris_manager::~polaris_manager()
{
	if (this->status != WFP_INIT_FAILED)
		this->client.deinit();
}

int polaris_manager::watch_service(const std::string& service_namespace,
								   const std::string& service_name)
{
	if (this->status == WFP_INIT_FAILED)
		return WFP_INIT_FAILED;

	PolarisTask *task;
	task = this->client.create_discover_task(service_namespace.c_str(),
											 service_name.c_str(),
											 this->retry_max,
											 this->discover_cb);

	WFFacilities::WaitGroup wait_group(1);
	struct watch_result result;
	result.wait_group = &wait_group;
	task->user_data = &result;
	task->set_config(this->config);

	struct series_context *context = new series_context();
	context->service_namespace = service_namespace;
	context->service_name = service_name;

	SeriesWork *series = Workflow::create_series_work(task,
												[](const SeriesWork *series) {
		delete (struct series_context *)series->get_context();
	});
	
	series->set_context(context);
	series->start();
	wait_group.wait();

	if (result.error == 0)
		++*this->ref;

	return result.error;
}

int polaris_manager::unwatch_service(const std::string& service_namespace,
									 const std::string& service_name)
{
	if (this->status == WFP_INIT_FAILED)
		return this->status;

	std::string policy_name = service_namespace + "." + service_name;

	std::unique_lock<std::mutex> lock(this->mutex);
	auto iter = this->watch_status.find(policy_name);

	if (iter == this->watch_status.end())
		return WFP_NO_WATCHING_SERVICE;

	if (iter->second.watching == true)
	{
		iter->second.watching = false;
		iter->second.cond.wait(lock);
		--*this->ref;
	}
	this->watch_status.erase(iter);

	PolarisPolicy *pp;
	pp = (PolarisPolicy *)WFGlobal::get_name_service()->del_policy(policy_name.c_str());
	this->unwatch_policies.emplace(policy_name, pp);

	return 0;
}

int polaris_manager::register_service(const std::string& service_namespace,
									  const std::string& service_name,
									  const PolarisInstance& instance)
{
	return 0;
}

int polaris_manager::deregister_service(const std::string& service_namespace,
										const std::string& service_name,
										const PolarisInstance& instance)
{
	return 0;
}

void polaris_manager::get_watching_list(std::vector<std::string>& list)
{
	this->mutex.lock();
	for (const auto &kv : this->watch_status)
		list.push_back(kv.first);
	this->mutex.unlock();
}

void polaris_manager::discover_callback(PolarisTask *task)
{
	int state = task->get_state();
	int error = task->get_error();
	struct watch_result *result = NULL;

	if (task->user_data)
	{
		result = (struct watch_result *)task->user_data;
		result->error = error;
	}

	struct discover_result discover;
	struct route_result route;
	bool update_instance = false;
	bool update_routing = false;

	if (state == WFT_STATE_SUCCESS)
	{
		update_instance = task->get_discover_result(&discover);
		update_routing = task->get_route_result(&route);
	}

	if (state != WFT_STATE_SUCCESS || !update_instance || !update_routing)
	{
		if (result)
		{
			result->wait_group->done();
			return;
		}
	}

	struct series_context *context =
			(struct series_context *)series_of(task)->get_context();
	std::string policy_name = context->service_namespace +
							  "." + context->service_name;

	this->mutex.lock();
	auto iter = this->watch_status.find(policy_name);

	if (iter != this->watch_status.end())
	{
		if (result)
		{
			result->error = WFP_DOUBLE_WATCH;
			this->mutex.unlock();
			result->wait_group->done();
			return;
		}

		if (!iter->second.watching)
		{
			iter->second.cond.notify_one();
			this->mutex.unlock();
			return;
		}

		if (update_instance)
			update_instance = (iter->second.service_revision != 
							   discover.service_revision);
		if (update_routing)
			update_routing = iter->second.routing_revision !=
							 route.routing_revision;
	}

	WFNameService *ns = WFGlobal::get_name_service();
	PolarisPolicy *pp;

	pp = dynamic_cast<PolarisPolicy *>(ns->get_policy(policy_name.c_str()));
	if (pp == NULL)
	{
		if (result)
		{
			auto iter = this->unwatch_policies.find(policy_name);
			if (iter == this->unwatch_policies.end())
			{
				PolarisPolicyConfig conf(policy_name);
				pp = new PolarisPolicy(&conf);
			}
			else
			{
				this->unwatch_policies.erase(iter);
				pp = iter->second;
			}

			ns->add_policy(policy_name.c_str(), pp);
		}
		else
		{
			result->error = WFP_EXISTED_POLICY;
			this->mutex.unlock();
			return;
		}
	}

	if (update_instance)
	{
		pp->update_instances(discover.instances);
		this->watch_status[policy_name].service_revision = discover.service_revision;
	}

	if (update_routing)
	{
		pp->update_inbounds(route.routing_inbounds);
		pp->update_outbounds(route.routing_outbounds);
		this->watch_status[policy_name].routing_revision = route.routing_revision;
	}

	this->watch_status[policy_name].watching = false;
	this->mutex.unlock();

	WFTimerTask *timer_task;
	unsigned int ms = this->config.get_discover_refresh_interval();

	timer_task = WFTaskFactory::create_timer_task(ms, this->timer_cb);
	series_of(task)->push_back(timer_task);

	if (result)
		result->wait_group->done();

	return;
}

void polaris_manager::timer_callback(WFTimerTask *task)
{
	struct series_context *context;
	context =(struct series_context *)series_of(task)->get_context();
	std::string policy_name = context->service_namespace +
							  "." + context->service_name;

	this->mutex.lock();
	auto iter = this->watch_status.find(policy_name);
	if (iter == this->watch_status.end())
	{
		if (--*this->ref == 0)
		{
			this->mutex.unlock();
			delete this->ref;
			delete this;
		}
		else
			this->mutex.unlock();

		return;
	}

	iter->second.watching = true;
	this->mutex.unlock();

	PolarisTask *discover_task;
	discover_task = this->client.create_discover_task(context->service_namespace.c_str(),
													  context->service_name.c_str(),
													  this->retry_max,
													  this->discover_cb);
	discover_task->set_config(this->config);
	series_of(task)->push_back(discover_task);
}

}; // namespace polaris

