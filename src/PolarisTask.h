#ifndef _POLARISTASK_H_
#define _POLARISTASK_H_

#include "workflow/WFTaskFactory.h"
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "src/PolarisConfig.h"
#include "src/PolarisCluster.h"
#include <stdlib.h>
#include <functional>

namespace polaris {

#define POLARIS_DISCOVER_CLUSTZER_INITED 1
#define POLARIS_HEALTHCHECK_CLUSTZER_INITED (1 << 1)

enum PolarisProtocol {
    P_UNKNOWN,
    P_HTTP,
    P_TRPC,
};

enum ApiType {
    DISCOVER = 1,
    REGISTER = 2,
    DEREGISTER = 3,
};

enum DiscoverRequestType {
    UNKNOWN = 0,
    INSTANCE,
    CLUSTER,
    ROUTING,
    RATE_LIMIT,
    CIRCUIT_BREAKER,
};
class PolarisTask;

using polaris_callback_t = std::function<void(PolarisTask *)>;
class PolarisTask : public WFGenericTask {
  public:
    PolarisTask(const std::string &service_namespace, const std::string &service_name,
                int retry_max, PolarisCluster *cluster, polaris_callback_t &&cb) {
        this->service_namespace = service_namespace;
        this->service_name = service_name;
        this->callback = std::move(cb);
        this->retry_max = retry_max;
        int pos = rand() % cluster->get_server_connectors()->size();
        this->url = cluster->get_server_connectors()->at(pos);
        this->cluster = *cluster;
    }

    void set_apitype(ApiType apitype) { this->apitype = apitype; }

    void set_protocol(PolarisProtocol protocol) { this->protocol = protocol; }

    void set_config(PolarisConfig config) { this->config = std::move(config); }

    void set_register_instance(RegisterInstance &instance) {
        this->reg_instance = std::move(instance);
    }

    bool get_discover_result(struct discover_result *result);
    bool get_route_result(struct route_result *result);

  protected:
    ~PolarisTask(){};
    WFHttpTask *create_cluster_http_task();
    WFHttpTask *create_instances_http_task();
    WFHttpTask *create_route_http_task();
    WFHttpTask *create_register_http_task();
    WFHttpTask *create_deregister_http_task();

    static void cluster_http_callback(WFHttpTask *task);
    static void instances_http_callback(WFHttpTask *task);
    static void route_http_callback(WFHttpTask *task);
    static void register_http_callback(WFHttpTask *task);

    std::string create_discover_request(const struct discover_request &request);
    std::string create_register_request(const struct register_request &request);
    std::string create_deregister_request(const struct deregister_request &request);

    bool parse_cluster_response(const std::string &body, std::string &revision);
    bool parse_instances_response(const std::string &body, std::string &revision);
    bool parse_route_response(const std::string &body, std::string &revision);
    bool parse_register_response(const std::string &body);

    virtual void dispatch();
    virtual SubTask *done();

    polaris_callback_t callback;

  private:
    std::string url;
    std::string service_namespace;
    std::string service_name;
    int retry_max;
    bool finish;
    ApiType apitype;
    PolarisProtocol protocol;
    std::string discover_res;
    std::string route_res;
    RegisterInstance reg_instance;
    PolarisConfig config;
    PolarisCluster cluster;
};

};  // namespace polaris

#endif
