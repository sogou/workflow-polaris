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

    ~PolarisTask(){};

    void set_apitype(ApiType apitype) { this->apitype = apitype; }

    void set_protocol(PolarisProtocol protocol) { this->protocol = protocol; }

    void set_config(PolarisConfig config) { this->config = std::move(config); }

    const discover_response_t *get_instances() const { return &this->instances; }
    const route_response_t *get_route() const { return &this->route; }

  protected:
    WFHttpTask *create_cluster_http_task();
    WFHttpTask *create_instances_http_task();
    WFHttpTask *create_route_http_task();

    static void polaris_cluster_http_callback(WFHttpTask *task);
    static void polaris_instances_http_callback(WFHttpTask *task);
    static void polaris_route_http_callback(WFHttpTask *task);

    std::string create_discover_request(const discover_request_t &request);
    bool parse_cluster_response(const json &j, std::string &revision);
    bool parse_instances_response(const json &j, std::string &revision);
    bool parse_route_response(const json &j, std::string &revision);

    // todo: check json field exists, now alawys return true
    bool check_json(const json &j) { return true; }
    virtual void dispatch();
    virtual SubTask *done();

    polaris_callback_t callback;

  private:
    std::string url;
    std::string service_namespace;
    std::string service_name;
    struct discover_response instances;
    struct route_response route;
    bool finish;
    int retry_max;
    ApiType apitype;
    PolarisProtocol protocol;
    PolarisConfig config;
    PolarisCluster cluster;
};

};  // namespace polaris

#endif
