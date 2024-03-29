#ifndef _POLARISCLIENT_H_
#define _POLARISCLIENT_H_

#include <string>
#include <functional>
#include "PolarisTask.h"
#include "PolarisCluster.h"

namespace polaris {

class PolarisClient {
  public:
    PolarisClient();

    int init(const std::string &url);

    PolarisTask *create_discover_task(const std::string &service_namespace,
                                      const std::string &service_name, int retry,
                                      polaris_callback_t cb);

    PolarisTask *create_register_task(const std::string &service_namespace,
                                      const std::string &service_name, int retry,
                                      polaris_callback_t cb);

    PolarisTask *create_deregister_task(const std::string &service_namespace,
                                        const std::string &service_name, int retry,
                                        polaris_callback_t cb);

	PolarisTask *create_ratelimit_task(const std::string &service_namespace,
                                      const std::string &service_name, int retry,
                                      polaris_callback_t cb);

	PolarisTask *create_circuitbreaker_task(const std::string &service_namespace,
                                      const std::string &service_name, int retry,
                                      polaris_callback_t cb);

	PolarisTask *create_heartbeat_task(const std::string &service_namespace,
                                       const std::string &service_name, int retry,
                                       polaris_callback_t cb);

  public:
    virtual ~PolarisClient();
    void deinit();

  private:
    PolarisProtocol protocol;
    PolarisCluster *cluster;
    friend class PolarisTask;
};

};  // namespace polaris

#endif
