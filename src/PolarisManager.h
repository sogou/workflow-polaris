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
#include "PolarisPolicy.h"

namespace polaris {

class Manager;

class PolarisManager
{
public:
	PolarisManager(const std::string& polaris_url);
	PolarisManager(const std::string& polaris_url,
				   const std::string& yaml_file);
	~PolarisManager();

	int watch_service(const std::string& service_namespace,
					  const std::string& service_name);
	int unwatch_service(const std::string& service_namespace,
						const std::string& service_name);

	int register_service(const std::string& service_namespace,
						 const std::string& service_name,
						 PolarisInstance instance);
	int register_service(const std::string& service_namespace,
						 const std::string& service_name,
						 const std::string& service_token,
						 PolarisInstance instance);
	int deregister_service(const std::string& service_namespace,
						   const std::string& service_name,
						   PolarisInstance instance);
	int deregister_service(const std::string& service_namespace,
						   const std::string& service_name,
						   const std::string& service_token,
						   PolarisInstance instance);
	int get_error() const;
	void get_watching_list(std::vector<std::string>& list);

private:
	Manager *ptr;
};

}; // namespace polaris

#endif

