#ifndef _POLARISMANAGER_H_
#define _POLARISMANAGER_H_

#include <mutex>
#include <string>
#include <functional>
#include <condition_variable>
#include "workflow/WFTask.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
#include "PolarisClient.h"
#include "PolarisPolicies.h"

namespace polaris {

enum 
{
	WFP_INIT_FAILED					=	-1,
	WFP_INIT_SUCCESS				=	0,
/*
	WFP_PARSE_CLUSTER_ERROR			=	1,
	WFP_PARSE_INSTANCES_ERROR		=	2,
	WFP_PARSE_ROUTE_ERROR			=	3,
	WFP_PARSE_REGISTER_ERROR		=	4,
	WFP_PARSE_RATELIMIT_ERROR		=	5,
	WFP_PARSE_CIRCUITBREAKER_ERROR	=	6,
*/
	WFP_DOUBLE_WATCH				=	21,
	WFP_EXISTED_SERVICE				=	22,
	WFP_NO_SERVICE					=	22,
};

class PolarisManager
{
public:
	PolarisManager(const std::string& polaris_url);
	PolarisManager(const std::string& polaris_url,
				   PolarisConfig config);
	~PolarisManager();

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

	std::vector<std::string> get_watching_list();

private:
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
	int status;

	std::function<void (PolarisTask *task)> discover_cb;
	std::function<void (WFTimerTask *task)> timer_cb;

private:
	void discover_callback(PolarisTask *task);
	void register_callback(PolarisTask *task);
	void deregister_callback(PolarisTask *task);
	void timer_callback(WFTimerTask *task);
};

}; // namespace polaris

#endif

