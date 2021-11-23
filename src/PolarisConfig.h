#ifndef _POLARISCONFIG_H_
#define _POLARISCONFIG_H_

#include <string>
#include <atomic>
#include <map>
#include <vector>

namespace polaris {

#define DEFAULT_NAMESPACE "Polaris"
#define DEFAULT_NAME "default"

struct polaris_config {
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
    // state report config
    bool state_report_enable;
    int state_report_windows_seconds;
    // circuitBreaker config
    // ratelimiter config
};

struct discover_request {
    int type;
    std::string service_name;
    std::string service_namespace;
    std::string revision;
};

struct instance {
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
    int healthcheck_type;
    int healthcheck_ttl;
    bool enable_healthcheck;
    bool healthy;
    bool isolate;
    std::string logic_set;
    std::string mtime;
    std::string revision;
    std::string region;
    std::string zone;
    std::string campus;
    std::map<std::string, std::string> metadata;
};

struct discover_result {
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
};

struct meta_label {
    std::string type;
    std::string value;
};

struct source_bound {
    std::string service;
    std::string service_namespace;
    std::map<std::string, struct meta_label> metadata;
};

struct destination_bound {
    std::string service;
    std::string service_namespace;
    std::map<std::string, struct meta_label> metadata;
    int priority;
    int weight;
};

struct bound {
    std::vector<struct source_bound> source_bounds;
    std::vector<struct destination_bound> destination_bounds;
};

struct route_result {
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
};

struct register_request {
    std::string service;
    std::string service_namespace;
    struct instance inst;
};

struct deregister_request {
    std::string id;
    std::string service;
    std::string service_namespace;
    std::string host;
    int port;
};

class PolarisInstance {
  public:
    PolarisInstance() {
        this->ref = new std::atomic<int>(1);
        this->inst = new instance;
        instance_init();
    }

    PolarisInstance(PolarisInstance &&move) {
        this->ref = move.ref;
        this->inst = move.inst;
        move.ref = new std::atomic<int>(1);
        move.inst = new instance;
        move.instance_init();
    }

    PolarisInstance &operator=(PolarisInstance &copy) {
        this->~PolarisInstance();
        this->ref = copy.ref;
        this->inst = copy.inst;
        ++*this->ref;
        return *this;
    }

    PolarisInstance &operator=(PolarisInstance &&move) {
        if (this != &move) {
            this->~PolarisInstance();
            this->ref = move.ref;
            this->inst = move.inst;
            move.ref = new std::atomic<int>(1);
            move.inst = new instance;
            move.instance_init();
        }
        return *this;
    }

    virtual ~PolarisInstance() {
        if (--*this->ref == 0) {
            delete this->ref;
            delete this->inst;
        }
    }

    void instance_init() {
        this->inst->enable_healthcheck = false;
        this->inst->healthy = true;
        this->inst->isolate = false;
        this->inst->weight = 100;
        this->inst->healthcheck_type = 1;
        this->inst->healthcheck_ttl = 5;
    }

    void set_id(const std::string &id) { this->inst->id = id; }
    void set_service(const std::string &service) { this->inst->service = service; }
    void set_service_namespace(const std::string &ns) { this->inst->service_namespace = ns; }
    void set_host(const std::string &host) { this->inst->host = host; }
    void set_port(int port) { this->inst->port = port; }
    void set_protocol(const std::string &protocol) { this->inst->protocol = protocol; }
    void set_version(const std::string &version) { this->inst->version = version; }
    void set_region(const std::string &region) { this->inst->region = region; }
    void set_zone(const std::string &zone) { this->inst->zone = zone; }
    void set_campus(const std::string &campus) { this->inst->campus = campus; }
    void set_weight(int weight) {
        if (weight >= 0 && weight <= 100) {
            this->inst->weight = weight;
        }
    }
    void set_enable_healthcheck(bool enable) { this->inst->enable_healthcheck = enable; }
    void set_healthcheck_ttl(int ttl) { this->inst->healthcheck_ttl = ttl; }
    void set_isolate(bool isolate) { this->inst->isolate = isolate; }
    void set_healthy(bool healthy) { this->inst->healthy = healthy; }
    void set_logic_set(const std::string &logic_set) { this->inst->logic_set = logic_set; }
    void set_metadata(const std::map<std::string, std::string> &metadata) {
        this->inst->metadata = metadata;
    }

    const struct instance *get_instance() const { return this->inst; }

  private:
    std::atomic<int> *ref;
    struct instance *inst;
};

class PolarisConfig {
  public:
    PolarisConfig() {
        this->ptr = new polaris_config;
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
        move.ptr = new polaris_config;
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
            move.ptr = new polaris_config;
            move.polaris_config_init();
        }
        return *this;
    }

    const struct polaris_config *get_polaris_config() const { return this->ptr; }

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
    struct polaris_config *ptr;
};

};  // namespace polaris

#endif
