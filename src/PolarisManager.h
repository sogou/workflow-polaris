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
	WFP_EXISTED_POLICY				=	21,
	WFP_DOUBLE_WATCH				=	22,
	WFP_NO_WATCHING_SERVICE			=	23,
	WFP_MANAGER_EXITED				=	24,
};

class Manager;

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

	void get_watching_list(std::vector<std::string>& list);

private:
	Manager *ptr;
};

}; // namespace polaris

#endif

