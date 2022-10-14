#include "PolarisManager.h"

namespace polaris {

#define RETRY_MAX	2

class Manager
{
public:
	Manager(const std::string& polaris_url, PolarisConfig config);
	~Manager();

	int watch_service(const std::string& service_namespace,
					  const std::string& service_name);
	int unwatch_service(const std::string& service_namespace,
						const std::string& service_name);

	int register_service(const std::string& service_namespace,
						 const std::string& service_name,
						 const std::string& service_token,
						 int heartbeat_interval,
						 PolarisInstance instance);
	int deregister_service(const std::string& service_namespace,
						   const std::string& service_name,
						   const std::string& service_token,
						   PolarisInstance instance);

	int get_error() const { return this->error; }
	void get_watching_list(std::vector<std::string>& list);
	void get_register_list(std::vector<std::string>& list);

public:
	void exit_locked();
	void incref() { ++this->ref; }
	void decref()
	{
		if (--this->ref == 0)
			delete this;
	}

private:
	std::atomic<int> ref;
	int retry_max;
	int error;
	std::string polaris_url;
	PolarisConfig config;
	PolarisClient client;

	struct watch_info
	{
		bool watching;
		std::string service_revision;
		std::string routing_revision;
		std::condition_variable cond;
	};
	struct register_info
	{
		bool heartbeating;
		std::condition_variable cond;
	};
	std::mutex mutex;
	std::unordered_map<std::string, struct watch_info> watch_status;
	std::unordered_map<std::string, PolarisPolicy *> unwatch_policies;
	std::unordered_map<std::string, struct register_info> register_status;

	enum
	{
		INIT_SUCCESS	=	0,
		INIT_FAILED		=	1,
		MANAGER_EXITED	=	2,
	};
	int status;

	std::function<void (PolarisTask *task)> discover_cb;
	std::function<void (WFTimerTask *task)> discover_timer_cb;
	std::function<void (PolarisTask *task)> register_cb;
	std::function<void (PolarisTask *task)> deregister_cb;
	std::function<void (PolarisTask *task)> heartbeat_cb;
	std::function<void (WFTimerTask *task)> heartbeat_timer_cb;

private:
	void set_error(int state, int error);
	bool update_policy_locked(const std::string& policy_name,
							  struct discover_result *discover,
							  struct route_result *route,
							  bool is_user_request,
							  bool update_instance,
							  bool update_routing);
	bool update_heartbeat_locked(const std::string& instance_name,
								 bool is_user_request);

	void discover_callback(PolarisTask *task);
	void register_callback(PolarisTask *task);
	void deregister_callback(PolarisTask *task);
	void heartbeat_callback(PolarisTask *task);
	void discover_timer_callback(WFTimerTask *task);
	void heartbeat_timer_callback(WFTimerTask *task);
};

struct consumer_context
{
	std::string service_namespace;
	std::string service_name;
	Manager *mgr;
};

struct provider_context
{
	std::string service_namespace;
	std::string service_name;
	std::string service_token;
	int heartbeat_interval;
	PolarisInstance instance;
	Manager *mgr;
};

struct deregister_context
{
	deregister_context() : wait_group(1) { }

	WFFacilities::WaitGroup wait_group;
	std::string instance_name;
};

PolarisManager::PolarisManager(const std::string& polaris_url)
{
	PolarisConfig config;
	this->ptr = new Manager(polaris_url, std::move(config));
}

PolarisManager::PolarisManager(const std::string& polaris_url,
							   const std::string& yaml_file)
{
	PolarisConfig config;
	config.init_from_yaml(yaml_file);
	this->ptr = new Manager(polaris_url, std::move(config));
}

PolarisManager::~PolarisManager()
{
	this->ptr->exit_locked();
}

int PolarisManager::get_error() const
{
	return this->ptr->get_error();
}

void Manager::exit_locked()
{
	bool flag = false;

	this->mutex.lock();
	if (--this->ref == 0)
		flag = true;
	else
		this->status = MANAGER_EXITED;
	this->mutex.unlock();

	if (flag)
		delete this;
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
									 PolarisInstance instance)
{
	return this->ptr->register_service(service_namespace, service_name,
									   "" , 0, std::move(instance));
}

int PolarisManager::register_service(const std::string& service_namespace,
									 const std::string& service_name,
									 const std::string& service_token,
									 PolarisInstance instance)
{
	return this->ptr->register_service(service_namespace, service_name,
									   service_token, 0, std::move(instance));
}

int PolarisManager::register_service(const std::string& service_namespace,
									 const std::string& service_name,
									 const std::string& service_token,
									 int heartbeat_interval,
									 PolarisInstance instance)
{
	return this->ptr->register_service(service_namespace, service_name,
									   service_token, heartbeat_interval,
									   std::move(instance));
}

int PolarisManager::deregister_service(const std::string& service_namespace,
									   const std::string& service_name,
									   PolarisInstance instance)
{
	return this->ptr->deregister_service(service_namespace, service_name,
										 "", std::move(instance));
}

int PolarisManager::deregister_service(const std::string& service_namespace,
									   const std::string& service_name,
									   const std::string& service_token,
									   PolarisInstance instance)
{
	return this->ptr->deregister_service(service_namespace, service_name,
										 service_token, std::move(instance));
}

void PolarisManager::get_watching_list(std::vector<std::string>& list)
{
	this->ptr->get_watching_list(list);
}

void PolarisManager::get_register_list(std::vector<std::string>& list)
{
	this->ptr->get_register_list(list);
}

Manager::Manager(const std::string& polaris_url, PolarisConfig config) :
	ref(1),
	error(0),
	polaris_url(polaris_url),
	config(std::move(config))
{
	if (client.init(polaris_url) == 0)
		this->status = INIT_SUCCESS;
	else
	{
		this->status = INIT_FAILED;
		this->error = POLARIS_ERR_INIT_FAILED;
	}

	this->retry_max = RETRY_MAX;

	this->discover_cb = std::bind(&Manager::discover_callback,
								  this, std::placeholders::_1);
	this->discover_timer_cb = std::bind(&Manager::discover_timer_callback,
										this, std::placeholders::_1);
	this->register_cb = std::bind(&Manager::register_callback,
								  this, std::placeholders::_1);
	this->deregister_cb = std::bind(&Manager::deregister_callback,
								    this, std::placeholders::_1);
	this->heartbeat_cb = std::bind(&Manager::heartbeat_callback,
								   this, std::placeholders::_1);
	this->heartbeat_timer_cb = std::bind(&Manager::heartbeat_timer_callback,
										 this, std::placeholders::_1);
}

Manager::~Manager()
{
	if (this->status != INIT_FAILED)
		this->client.deinit();
}

int Manager::watch_service(const std::string& service_namespace,
						   const std::string& service_name)
{
	if (this->status == INIT_FAILED)
		return -1;

	PolarisTask *task;
	task = this->client.create_discover_task(service_namespace.c_str(),
											 service_name.c_str(),
											 this->retry_max,
											 this->discover_cb);

	WFFacilities::WaitGroup wait_group(1);
	task->user_data = &wait_group;
	task->set_config(this->config);

	struct consumer_context *ctx = new consumer_context();
	ctx->service_namespace = service_namespace;
	ctx->service_name = service_name;
	ctx->mgr = this;
	this->incref();

	SeriesWork *series = Workflow::create_series_work(task,
											[](const SeriesWork *series) {
		struct consumer_context *ctx;
		ctx = (struct consumer_context *)series->get_context();
		ctx->mgr->decref();
		delete ctx;
	});

	series->set_context(ctx);
	series->start();
	wait_group.wait();

	return this->error >= 0 ? 0 : -1;
}

int Manager::unwatch_service(const std::string& service_namespace,
							 const std::string& service_name)
{
	if (this->status == INIT_FAILED)
		return -1;

	std::string policy_name = service_namespace + "." + service_name;

	std::unique_lock<std::mutex> lock(this->mutex);
	auto iter = this->watch_status.find(policy_name);

	if (iter == this->watch_status.end())
	{
		this->error = POLARIS_ERR_SERVICE_NOT_FOUND;
		return -1;
	}

	if (iter->second.watching == true)
	{
		iter->second.watching = false;
		iter->second.cond.wait(lock);
	}
	this->watch_status.erase(iter);

	PolarisPolicy *pp;
	pp = (PolarisPolicy *)WFGlobal::get_name_service()->del_policy(policy_name.c_str());
	this->unwatch_policies.emplace(policy_name, pp);

	return 0;
}

int Manager::register_service(const std::string& service_namespace,
							  const std::string& service_name,
							  const std::string& service_token,
							  int heartbeat_interval,
							  PolarisInstance instance)
{
	if (this->status == INIT_FAILED)
		return -1;

	PolarisTask *task;
	task = this->client.create_register_task(service_namespace.c_str(),
											 service_name.c_str(),
											 this->retry_max,
											 this->register_cb);
	if (!service_token.empty())
		task->set_service_token(service_token);

	WFFacilities::WaitGroup wait_group(1);
	task->user_data = &wait_group;
	task->set_config(this->config);
	task->set_polaris_instance(instance);

	struct provider_context *ctx = new provider_context();
	ctx->service_namespace = service_namespace;
	ctx->service_name = service_name;
	ctx->service_token = service_token;
	ctx->heartbeat_interval = heartbeat_interval;
	ctx->instance = std::move(instance);
	ctx->mgr = this;
	this->incref();

	SeriesWork *series = Workflow::create_series_work(task,
											[](const SeriesWork *series) {
		struct provider_context *ctx;
		ctx = (struct provider_context *)series->get_context();
		ctx->mgr->decref();
		delete ctx;
	});

	series->set_context(ctx);
	series->start();
	wait_group.wait();

	return this->error >= 0 ? 0 : -1;
}

int Manager::deregister_service(const std::string& service_namespace,
								const std::string& service_name,
								const std::string& service_token,
								PolarisInstance instance)
{
	if (this->status == INIT_FAILED)
		return -1;

	std::string inst = instance.get_host() + ":" +
					   std::to_string(instance.get_port());

	this->mutex.lock();
	if (this->register_status.find(inst) == this->register_status.end())
	{
		this->error = POLARIS_ERR_SERVICE_NOT_FOUND;
		this->mutex.unlock();
		return -1;
	}
	this->mutex.unlock();

	PolarisTask *task;
	task = this->client.create_deregister_task(service_namespace.c_str(),
											   service_name.c_str(),
											   this->retry_max,
											   this->deregister_cb);
	if (!service_token.empty())
		task->set_service_token(service_token);

	struct deregister_context *ctx = new deregister_context();
	ctx->instance_name = std::move(inst);

	task->user_data = ctx;
	task->set_config(this->config);
	task->set_polaris_instance(std::move(instance));
	task->start();
	ctx->wait_group.wait();

	return this->error == 0 ? 0 : -1;
}

void Manager::get_watching_list(std::vector<std::string>& list)
{
	this->mutex.lock();
	for (const auto &kv : this->watch_status)
		list.push_back(kv.first);
	this->mutex.unlock();
}

void Manager::get_register_list(std::vector<std::string>& list)
{
	this->mutex.lock();
	for (const auto &kv : this->register_status)
		list.push_back(kv.first);
	this->mutex.unlock();
}

void Manager::set_error(int state, int error)
{
	switch (state)
	{
	case POLARIS_STATE_ERROR:
		this->error = error;
		break;
	case WFT_STATE_SYS_ERROR:
		this->error = POLARIS_ERR_SYS_ERROR;
		errno = error;
		break;
	case WFT_STATE_SSL_ERROR:
		this->error = POLARIS_ERR_SSL_ERROR;
		break;
	case WFT_STATE_DNS_ERROR:
		this->error = POLARIS_ERR_DNS_ERROR;
		break;
	case WFT_STATE_TASK_ERROR:
		this->error = POLARIS_ERR_TASK_ERROR;
		break;
	default:
		this->error = POLARIS_ERR_UNKNOWN_ERROR;
		break;
	}
}

bool Manager::update_policy_locked(const std::string& policy_name,
								   struct discover_result *discover,
								   struct route_result *route,
								   bool is_user_request,
								   bool update_instance,
								   bool update_routing)
{
	auto iter = this->watch_status.find(policy_name);

	if (iter != this->watch_status.end())
	{
		if (is_user_request)
		{
			this->error = POLARIS_ERR_DOUBLE_OPERATION;
			return false;
		}

		if (!iter->second.watching)
		{
			iter->second.cond.notify_one();
			return false;
		}

		if (update_instance)
			update_instance = (iter->second.service_revision != 
							   discover->service_revision);
		if (update_routing)
			update_routing = (iter->second.routing_revision !=
							  route->routing_revision);
	}

	WFNameService *ns = WFGlobal::get_name_service();
	PolarisPolicy *pp;

	pp = dynamic_cast<PolarisPolicy *>(ns->get_policy(policy_name.c_str()));
	if (pp == NULL)
	{
		if (is_user_request)
		{
			auto iter = this->unwatch_policies.find(policy_name);
			if (iter == this->unwatch_policies.end())
			{
				PolarisPolicyConfig conf(policy_name, this->config);
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
			this->error = POLARIS_ERR_EXISTED_POLICY;
			return false;
		}
	}

	if (update_instance)
	{
		pp->update_instances(discover->instances);
		this->watch_status[policy_name].service_revision = discover->service_revision;
	}

	if (update_routing)
	{
		pp->update_inbounds(route->routing_inbounds);
		pp->update_outbounds(route->routing_outbounds);
		this->watch_status[policy_name].routing_revision = route->routing_revision;
	}

	this->watch_status[policy_name].watching = false;
	return true;
}

void Manager::discover_callback(PolarisTask *task)
{
	if (this->status == MANAGER_EXITED)
		return;

	int state = task->get_state();
	int error = task->get_error();

	struct discover_result discover;
	struct route_result route;
	struct consumer_context *ctx;
	bool update_instance = false;
	bool update_routing = false;
	bool ret;

	if (state == WFT_STATE_SUCCESS)
	{
		update_instance = task->get_discover_result(&discover);
		update_routing = task->get_route_result(&route);
	}

	if (task->user_data)
	{
		if (state != WFT_STATE_SUCCESS)
			this->set_error(state, error);
		else
		{
			if (!update_instance)
				this->error = POLARIS_ERR_NO_INSTANCE;
			else if (!update_routing)
				this->error = POLARIS_ERR_INVALID_ROUTE_RULE;
		}

		if (this->error != 0)
		{
			((WFFacilities::WaitGroup *)task->user_data)->done();
			return;
		}
	}

	ctx = (struct consumer_context *)series_of(task)->get_context();
	std::string policy_name = ctx->service_namespace +
							  "." + ctx->service_name;

	this->mutex.lock();
	ret = this->update_policy_locked(policy_name, &discover, &route,
									 task->user_data ? true : false,
									 update_instance, update_routing);
	this->mutex.unlock();

	if (ret == true)
	{
		WFTimerTask *timer_task;
		unsigned int us = this->config.get_discover_refresh_interval() * 1000;
		timer_task = WFTaskFactory::create_timer_task(us, this->discover_timer_cb);
		series_of(task)->push_back(timer_task);
	}

	if (task->user_data)
		((WFFacilities::WaitGroup *)task->user_data)->done();

	return;
}

void Manager::discover_timer_callback(WFTimerTask *task)
{
	struct consumer_context *ctx;
	ctx =(struct consumer_context *)series_of(task)->get_context();
	std::string policy_name = ctx->service_namespace + "." + ctx->service_name;

	if (this->status == MANAGER_EXITED)
		return;

	this->mutex.lock();
	auto iter = this->watch_status.find(policy_name);
	if (iter == this->watch_status.end())
	{
		this->mutex.unlock();
		return;
	}

	iter->second.watching = true;
	this->mutex.unlock();

	PolarisTask *discover_task;
	discover_task = this->client.create_discover_task(ctx->service_namespace.c_str(),
													  ctx->service_name.c_str(),
													  this->retry_max,
													  this->discover_cb);
	discover_task->set_config(this->config);
	series_of(task)->push_back(discover_task);
}

void Manager::register_callback(PolarisTask *task)
{
	int state = task->get_state();
	int error = task->get_error();

	PolarisTask *heartbeat_task;
	struct provider_context *ctx;

	ctx = (struct provider_context *)series_of(task)->get_context();

	if (state != WFT_STATE_SUCCESS)
	{
		this->set_error(state, error);
		((WFFacilities::WaitGroup *)task->user_data)->done();
		return;
	}

	if (ctx->instance.get_enable_healthcheck() && ctx->heartbeat_interval != 0)
	{
		heartbeat_task = this->client.create_heartbeat_task(
												ctx->service_namespace.c_str(),
												ctx->service_name.c_str(),
												this->retry_max,
												this->heartbeat_cb);

		heartbeat_task->set_config(this->config);
		heartbeat_task->set_polaris_instance(ctx->instance);
		heartbeat_task->set_service_token(ctx->service_token);

		heartbeat_task->user_data = task->user_data;
		series_of(task)->push_back(heartbeat_task);
	}
	else
	{
		std::string instance = ctx->instance.get_host() + ":" +
							   std::to_string(ctx->instance.get_port());

		this->mutex.lock();
		this->register_status[instance].heartbeating = false;
		this->mutex.unlock();
	}

	return;
}

void Manager::deregister_callback(PolarisTask *task)
{
	struct deregister_context *ctx = (struct deregister_context *)task->user_data;
	std::unique_lock<std::mutex> lock(this->mutex);

	auto iter = this->register_status.find(ctx->instance_name);

	if (iter != this->register_status.end())
	{
		if (iter->second.heartbeating == true)
		{
			iter->second.heartbeating = false;
			iter->second.cond.wait(lock);
		}
		this->register_status.erase(iter);
	}

	ctx->wait_group.done();
	delete ctx;

	return;
}

void Manager::heartbeat_callback(PolarisTask *task)
{
	if (this->status == MANAGER_EXITED)
		return;

	int state = task->get_state();
	int error = task->get_error();

	WFTimerTask *timer_task;
	struct provider_context *ctx;
	bool ret = true;

	if (task->user_data &&
		state != WFT_STATE_SUCCESS &&
		error == POLARIS_ERR_HEARTBEAT_DISABLE)
	{
		// will continue even if the first heartbeat network failed
		this->set_error(state, error);
		ret = false;
	}

	if (ret == true)
	{
		ctx = (struct provider_context *)series_of(task)->get_context();
		std::string instance = ctx->instance.get_host() + ":" +
							   std::to_string(ctx->instance.get_port());

		this->mutex.lock();
		ret = this->update_heartbeat_locked(instance, task->user_data ? true : false);
		this->mutex.unlock();

		if (ret == true)
		{
			timer_task = WFTaskFactory::create_timer_task(ctx->heartbeat_interval, 0,
														  this->heartbeat_timer_cb);
			series_of(task)->push_back(timer_task);
		}
	}

	if (task->user_data)
		((WFFacilities::WaitGroup *)task->user_data)->done();

	return;
}

bool Manager::update_heartbeat_locked(const std::string& instance_name,
									  bool is_user_request)
{
	auto iter = this->register_status.find(instance_name);

	if (iter != this->register_status.end())
	{
		if (is_user_request)
		{
			this->error = POLARIS_ERR_DOUBLE_OPERATION;
			return false;
		}

		if (!iter->second.heartbeating) // some one calling deregister()
		{
			iter->second.cond.notify_one();
			return false;
		}
	}

	this->register_status[instance_name].heartbeating = false;
	return true;
}

void Manager::heartbeat_timer_callback(WFTimerTask *task)
{
	struct provider_context *ctx;
	ctx =(struct provider_context *)series_of(task)->get_context();
	std::string instance = ctx->instance.get_host() + ":" +
						   std::to_string(ctx->instance.get_port());

	if (this->status == MANAGER_EXITED)
		return;

	this->mutex.lock();
	auto iter = this->register_status.find(instance);
	if (iter == this->register_status.end())
	{
		this->mutex.unlock();
		return;
	}

	iter->second.heartbeating = true;
	this->mutex.unlock();

	PolarisTask *heartbeat_task;
	heartbeat_task = this->client.create_heartbeat_task(
											ctx->service_namespace.c_str(),
											ctx->service_name.c_str(),
											this->retry_max,
											this->heartbeat_cb);

	heartbeat_task->set_config(this->config);
	heartbeat_task->set_service_token(ctx->service_token);
	heartbeat_task->set_polaris_instance(ctx->instance);
	series_of(task)->push_back(heartbeat_task);
}

}; // namespace polaris

