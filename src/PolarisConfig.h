#ifndef _POLARISCONFIG_H_
#define _POLARISCONFIG_H_

#include <string>
#include <atomic>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace polaris {

#define DEFAULT_NAMESPACE "Polaris"

typedef struct polaris_config {
    // system config
    std::string discover_namespace;
    std::string discover_name;
    int discover_refresh_seconds;
    std::string healthcheck_namespace;
    std::string healthcheck_name;
    int healthcheck_refresh_seconds;
    std::string monitor_namespace;
    std::string monitor_name;
    int monitor_refresh_seconds;
    std::string metrics_namespace;
    std::string metrics_name;
    int metrics_refresh_seconds;
    // consumer config
    int consumer_refresh_seconds;
    int consumer_expiretime_seconds;
    // api config
    std::string api_bindIf;
    std::string api_bindIP;
    int api_timeout_seconds;
    int api_retry_max;
    int api_retry_seconds;
    std::string api_location_region;
    std::string api_location_zone;
    std::string api_location_campus;
    // state report config
    bool state_report_enable;
    int state_report_windows_seconds;
    // circuitBreaker config
    // ratelimiter config
} polaris_config_t;

typedef struct discover_request {
    int type;
    std::string service_name;
    std::string service_namespace;
    std::string revision;
} discover_request_t;

typedef struct instance {
    std::string id;
    std::string service;
    std::string service_namespace;
    std::string vpc_id;
    std::string host;
    int port;
    std::string protocol;
    std::string version;
    int priority;
    int weight;
    bool enable_healthcheck;
    bool healthy;
    bool isolate;
    std::string metadata_protocol;
    std::string metadata_version;
    std::string logic_set;
    std::string mtime;
    std::string revision;
} instance_t;

typedef struct discover_result {
    int code;
    std::string info;
    std::string type;
    // service data
    std::string service_namespace;
    std::string service_name;
    std::string service_revision;
    std::map<std::string, std::string> service_metadata;
    std::string service_ports;
    std::string service_business;
    std::string service_department;
    std::string service_cmdbmod1;
    std::string service_cmdbmod2;
    std::string service_cmdbmod3;
    std::string service_comment;
    std::string service_owners;
    std::string service_ctime;
    std::string service_mtime;
    std::string service_platform_id;
    // instances
    std::vector<struct instance> instances;
} discover_result_t;

typedef struct label {
    std::string type;
    std::string value;
} label_t;

typedef struct source_bound {
    std::string service;
    std::string service_namespace;
    std::map<std::string, struct label> metadata;

} source_bound_t;

typedef struct destination_bound {
    std::string service;
    std::string service_namespace;
    std::map<std::string, struct label> metadata;
    int priority;
    int weight;
} destination_bound_t;

typedef struct bound {
    std::vector<struct source_bound> source_bounds;
    std::vector<struct destination_bound> destination_bounds;
} bound_t;

typedef struct route_result {
    int code;
    std::string info;
    std::string type;
    std::string service_name;
    std::string service_namespace;
    std::string routing_service;
    std::string routing_namespace;
    std::vector<struct bound> routing_inbounds;
    std::vector<struct bound> routing_outbounds;
    std::string routing_ctime;
    std::string routing_mtime;
    std::string routing_revision;
} route_result_t;

void to_json(json &j, const struct discover_request &request);
void from_json(const json &j, struct instance &response);
void from_json(const json &j, struct discover_result &response);
void from_json(const json &j, struct label &response);
void from_json(const json &j, struct source_bound &response);
void from_json(const json &j, struct destination_bound &response);
void from_json(const json &j, struct bound &response);
void from_json(const json &j, struct route_result &response);

class PolarisConfig {
  public:
    PolarisConfig() {
        this->ptr = new polaris_config_t;
        this->ref = new std::atomic<int>(1);
        polaris_config_init();
    }

    virtual ~PolarisConfig() {
        if (--*this->ref == 0) {
            delete this->ptr;
            delete this->ref;
        }
    }

    PolarisConfig(PolarisConfig &&move) {
        this->ptr = move.ptr;
        this->ref = move.ref;
        move.ref = new std::atomic<int>(1);
        move.ptr = new polaris_config_t;
        move.polaris_config_init();
    }

    PolarisConfig &operator=(const PolarisConfig &copy) {
        this->~PolarisConfig();
        this->ptr = copy.ptr;
        this->ref = copy.ref;
        ++*this->ref;
        return *this;
    }

    PolarisConfig &operator=(PolarisConfig &&move) {
        if (this != &move) {
            this->~PolarisConfig();
            this->ptr = move.ptr;
            this->ref = move.ref;
            move.ref = new std::atomic<int>(1);
            move.ptr = new polaris_config_t;
            move.polaris_config_init();
        }
        return *this;
    }

    std::string get_discover_namespace() { return ptr->discover_namespace; }

    std::string get_discover_name() { return ptr->discover_name; }

  protected:
    void polaris_config_init() {
        this->ptr->discover_namespace = DEFAULT_NAMESPACE;
        this->ptr->discover_name = "polaris.discover";
        this->ptr->discover_refresh_seconds = 600;  // 10 * 60 seconds
        this->ptr->healthcheck_namespace = DEFAULT_NAMESPACE;
        this->ptr->healthcheck_name = "polaris.healthcheck";
        this->ptr->healthcheck_refresh_seconds = 600;
        this->ptr->monitor_namespace = DEFAULT_NAMESPACE;
        this->ptr->monitor_name = "polaris.monitor";
        this->ptr->monitor_refresh_seconds = 600;
        this->ptr->metrics_namespace = DEFAULT_NAMESPACE;
        this->ptr->metrics_name = "polaris.metrics";
        this->ptr->metrics_refresh_seconds = 600;
        this->ptr->consumer_refresh_seconds = 5;
        this->ptr->consumer_expiretime_seconds = 600;
        this->ptr->api_bindIf = "eth0";
        this->ptr->api_bindIP = "127.0.0.1";
    }

  private:
    std::atomic<int> *ref;
    polaris_config_t *ptr;
};

};  // namespace polaris

#endif
