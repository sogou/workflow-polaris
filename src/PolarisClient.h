#ifndef _POLARISCLIENT_H_
#define _POLARISCLIENT_H_

#include <string>
#include <functional>
#include "src/PolarisTask.h"
#include "src/PolarisCluster.h"

namespace polaris {

class PolarisClient {
  public:
    PolarisClient();

    int init(const std::string &url);

    PolarisTask *create_discover_task(const std::string &service_namespace,
                                       const std::string &service_name, int retry,
                                       polaris_callback_t cb);

    // PolarisTask *create_register_task();

    // PolarisTask *create_deregister_task();

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
