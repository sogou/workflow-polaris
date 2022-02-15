#include "PolarisManager.h"

namespace polaris {

#define RETRY_MAX	2

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

PolarisManager::PolarisManager(const std::string& polaris_url) :
	polaris_url(polaris_url)
{
	if (client.init(polaris_url) == 0)
		this->status = WFP_INIT_SUCCESS;
	else
		this->status = WFP_INIT_FAILED;

	this->retry_max = RETRY_MAX;

	this->discover_cb = std::bind(&PolarisManager::discover_callback,
								  this,
								  std::placeholders::_1);

	this->timer_cb = std::bind(&PolarisManager::timer_callback,
							   this,
							   std::placeholders::_1);
}

PolarisManager::PolarisManager(const std::string& polaris_url,
							   PolarisConfig config) :
	polaris_url(polaris_url),
	config(std::move(config))
{
	if (client.init(polaris_url) == 0)
		this->status = WFP_INIT_SUCCESS;
	else
		this->status = WFP_INIT_FAILED;

	this->retry_max = RETRY_MAX;

	this->discover_cb = std::bind(&PolarisManager::discover_callback,
								  this,
								  std::placeholders::_1);

	this->timer_cb = std::bind(&PolarisManager::timer_callback,
							   this,
							   std::placeholders::_1);
}

PolarisManager::~PolarisManager()
{
	if (this->status != WFP_INIT_FAILED)
		this->client.deinit();
}

int PolarisManager::watch_service(const std::string& service_namespace,
								  const std::string& service_name)
{
	if (this->status == WFP_INIT_FAILED)
		return WFP_INIT_FAILED;

	PolarisTask *task = this->client.create_discover_task(service_namespace.c_str(),
														  service_name.c_str(),
														  this->retry_max,
														  this->discover_cb);

	WFFacilities::WaitGroup wait_group(1);
	struct watch_result result;
	result.wait_group = &wait_group;
	task->user_data = &result;
//	task->set_config(this->config);
	PolarisConfig config;
	task->set_config(std::move(config));

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

	return result.error;
}

int PolarisManager::unwatch_service(const std::string& service_namespace,
									const std::string& service_name)

{
	if (this->status == WFP_INIT_FAILED)
		return this->status;

	std::string policy_name = service_namespace + "." + service_name;

	std::unique_lock<std::mutex> lock(this->mutex);
	auto iter = this->watch_status.find(policy_name);

	if (iter == this->watch_status.end())
		return WFP_NO_SERVICE;

	if (iter->second.watching == true)
	{
		iter->second.watching = false;
		iter->second.cond.wait(lock);
	}
	this->watch_status.erase(iter);
	WFNSPolicy *pp = WFGlobal::get_name_service()->del_policy(policy_name.c_str());
	delete pp;

	return 0;
}

int PolarisManager::register_service(const std::string& service_namespace,
									 const std::string& service_name,
									 const PolarisInstance& instance)
{
	return 0;
}

int PolarisManager::deregister_service(const std::string& service_namespace,
									 	const std::string& service_name,
										const PolarisInstance& instance)
{
	return 0;
}

std::vector<std::string> PolarisManager::get_watching_list()
{
	std::vector<std::string> ret;

	this->mutex.lock();
	for (const auto &kv : this->watch_status)
		ret.push_back(kv.first);
	this->mutex.unlock();

	return ret;
}

void PolarisManager::discover_callback(PolarisTask *task)
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
			PolarisPolicyConfig conf(policy_name);
			pp = new PolarisPolicy(&conf);
			ns->add_policy(policy_name.c_str(), pp);
		}
		else
		{
			result->error = WFP_EXISTED_SERVICE;
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
	int ms = this->config.get_discover_refresh_seconds() * 1000;
	timer_task = WFTaskFactory::create_timer_task(ms, this->timer_cb);
	series_of(task)->push_back(timer_task);

	if (result)
		result->wait_group->done();

	return;
}

void PolarisManager::timer_callback(WFTimerTask *task)
{
	struct series_context *context;
	context =(struct series_context *)series_of(task)->get_context();
	std::string policy_name = context->service_namespace +
							  "." + context->service_name;

	std::lock_guard<std::mutex> lock(this->mutex);
	auto iter = this->watch_status.find(policy_name);
	if (iter == this->watch_status.end())
		return;

	iter->second.watching = true;
	this->mutex.unlock();

	PolarisTask *discover_task;
	discover_task = this->client.create_discover_task(context->service_namespace.c_str(),
													  context->service_name.c_str(),
													  this->retry_max,
													  this->discover_cb);
	PolarisConfig config;
	discover_task->set_config(std::move(config));
//	discover_task->set_config(this->config);
	series_of(task)->push_back(discover_task);
}

}; // namespace polaris

