#ifndef _POLARISTASK_H_
#define _POLARISTASK_H_

#include "workflow/WFTaskFactory.h"
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "PolarisConfig.h"
#include "PolarisCluster.h"
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
    API_UNKNOWN = 0,
    API_DISCOVER,
    API_REGISTER,
    API_DEREGISTER,
    API_RATELIMIT,
    API_CIRCUITBREAKER,
};

enum DiscoverRequestType {
    UNKNOWN = 0,
    INSTANCE,
    CLUSTER,
    ROUTING,
    RATELIMIT,
    CIRCUITBREAKER,
};

enum {
	WFT_STATE_POLARIS_SERVER_ERROR	=	1006, // kReturnServerError
};

enum {
	WFP_PARSE_CLUSTER_ERROR			=	1,
	WFP_PARSE_INSTANCES_ERROR		=	2,
	WFP_PARSE_ROUTE_ERROR			=	3,
	WFP_PARSE_REGISTER_ERROR		=	4,
	WFP_PARSE_RATELIMIT_ERROR		=	5,
	WFP_PARSE_CIRCUITBREAKER_ERROR	=	6,
};

class PolarisTask;

using polaris_callback_t = std::function<void(PolarisTask *)>;
class PolarisTask : public WFGenericTask {
  public:
    PolarisTask(const std::string &snamespace, const std::string &sname, int retry_max,
                PolarisCluster *cluster, polaris_callback_t &&cb)
        : service_namespace(snamespace),
          service_name(sname),
          retry_max(retry_max),
          callback(std::move(cb)) {
        this->apitype = API_UNKNOWN;
        this->protocol = P_UNKNOWN;
        int pos = rand() % cluster->get_server_connectors()->size();
        this->url = cluster->get_server_connectors()->at(pos);
        this->cluster = *cluster;
    }

    void set_apitype(ApiType apitype) { this->apitype = apitype; }

    void set_protocol(PolarisProtocol protocol) { this->protocol = protocol; }

    void set_config(PolarisConfig config) { this->config = std::move(config); }

    void set_service_token(const std::string &token) { this->service_token = token; }

    void set_polaris_instance(PolarisInstance instance) {
        this->polaris_instance = std::move(instance);
    }

    bool get_discover_result(struct discover_result *result) const;
    bool get_route_result(struct route_result *result) const;
    bool get_ratelimit_result(struct ratelimit_result *result) const;
    bool get_circuitbreaker_result(struct circuitbreaker_result *result) const;

  protected:
    virtual ~PolarisTask(){};
    WFHttpTask *create_cluster_http_task();
    WFHttpTask *create_instances_http_task();
    WFHttpTask *create_route_http_task();
    WFHttpTask *create_register_http_task();
    WFHttpTask *create_deregister_http_task();
    WFHttpTask *create_ratelimit_http_task();
    WFHttpTask *create_circuitbreaker_http_task();

    static void cluster_http_callback(WFHttpTask *task);
    static void instances_http_callback(WFHttpTask *task);
    static void route_http_callback(WFHttpTask *task);
    static void register_http_callback(WFHttpTask *task);
    static void ratelimit_http_callback(WFHttpTask *task);
    static void circuitbreaker_http_callback(WFHttpTask *task);

    std::string create_discover_request(const struct discover_request &request);
    std::string create_register_request(const struct register_request &request);
    std::string create_deregister_request(const struct deregister_request &request);
    std::string create_ratelimit_request(const struct ratelimit_request &request);
    std::string create_circuitbreaker_request(const struct circuitbreaker_request &request);

    bool parse_cluster_response(const std::string &body, std::string &revision);
    bool parse_instances_response(const std::string &body, std::string &revision);
    bool parse_route_response(const std::string &body, std::string &revision);
    bool parse_register_response(const std::string &body);
    bool parse_ratelimit_response(const std::string &body, std::string &revision);
    bool parse_circuitbreaker_response(const std::string &body, std::string &revision);

    virtual void dispatch();
    virtual SubTask *done();

  private:
    std::string service_namespace;
    std::string service_name;
    int retry_max;
    polaris_callback_t callback;
    std::string url;
    std::string service_token;
    bool finish;
    ApiType apitype;
    PolarisProtocol protocol;
    std::string discover_res;
    std::string route_res;
    std::string ratelimit_res;
    std::string circuitbreaker_res;
    PolarisInstance polaris_instance;
    PolarisConfig config;
    PolarisCluster cluster;
};

};  // namespace polaris

#endif
