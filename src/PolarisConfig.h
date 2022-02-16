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
    // router config
    // 基于主调和被调服务规则的路由策略
    bool rule_based_router;
    // 就近路由策略
    bool nearby_based_router;
    // 就近路由的最小匹配级别: region(大区)、zone(区域)、campus(园区)
    std::string nearby_match_level;
    // 就近路由最大的匹配级别
    std::string nearby_max_match_level;
    // 是否启用按服务不健康实例比例进行降级
    bool nearby_unhealthy_degrade;
    // 需要进行降级的实例比例，不健康实例达到百分之多少才进行降级
    int nearby_unhealthy_degrade_percent;
    // 允许全死全活
    bool nearby_enable_recover_all;
    // 是否开启元数据路由
    bool dst_meta_router;

    // api config
    std::string api_bindIf;
    std::string api_bindIP;
    std::string api_location_zone;
    std::string api_location_region;
    std::string api_location_campus;
    int api_timeout_seconds;
    int api_retry_max;
    int api_retry_seconds;
    // state report config
    bool state_report_enable;

    // circuitBreaker config
    // 是否启用节点熔断功能
    bool circuit_breaker_enable;
    // 实例定时熔断检测周期
    std::string circuit_breaker_check_period;
    // circuitBreaker plugin errorCount
    // 是否启用基于周期连续错误数熔断策略配置
    bool circuit_breaker_error_count;
    // 触发连续错误熔断的阈值
    int error_count_request_threshold;
    // 连续失败的统计周期
    std::string error_count_stat_time_window;
    // 熔断器打开后，多久后转换为半开状态
    std::string error_count_sleep_window;
    // 熔断器半开后最大允许的请求数
    int error_count_max_request_halfopen;
    // 熔断器半开到关闭所必须的最少成功请求数
    int error_count_least_success_halfopen;
    // circuitBreaker plugin errorRate
    // 基于周期错误率的熔断策略配置
    bool circuit_breaker_error_rate;
    // 触发错误率熔断的最低请求阈值
    int error_rate_request_threshold;
    // 触发错误率熔断的阈值
    double error_rate_threshold;
    // 错误率熔断的统计周期
    std::string error_rate_stat_time_window;
    // 错误率熔断的最小统计单元数量
    int error_rate_num_buckets;
    // 熔断器打开后，多久后转换为半开状态
    std::string error_rate_sleep_window;
    // 熔断器半开后最大允许的请求数
    int error_rate_max_request_halfopen;
    // 熔断器半开到关闭所必须的最少成功请求数
    int error_rate_least_success_halfopen;
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
    bool enable_healthcheck;
    std::string healthcheck_type;
    int healthcheck_ttl;
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
    std::string value_type;
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

struct routing_bound {
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
    std::vector<struct routing_bound> routing_inbounds;
    std::vector<struct routing_bound> routing_outbounds;
    std::string routing_ctime;
    std::string routing_mtime;
    std::string routing_revision;
};

struct register_request {
    std::string service;
    std::string service_namespace;
    std::string service_token;
    struct instance inst;
};

struct deregister_request {
    std::string id;
    std::string service;
    std::string service_namespace;
    std::string service_token;
    std::string host;
    int port;
};

struct ratelimit_request {
    int type;
    std::string service_name;
    std::string service_namespace;
    std::string revision;
};

struct ratelimit_amount {
    int max_amount;
    std::string valid_duration;
};

struct ratelimit_rule {
    std::string id;
    std::string service;
    std::string service_namespace;
    int priority;
    std::string type;
    std::map<std::string, struct meta_label> meta_labels;
    std::vector<struct ratelimit_amount> ratelimit_amounts;
    std::string action;
    bool disable;
    std::string ctime;
    std::string mtime;
    std::string revision;
};

struct ratelimit_result {
    int code;
    std::string info;
    std::string type;
    std::string service_name;
    std::string service_namespace;
    std::string service_revision;
    std::vector<struct ratelimit_rule> ratelimit_rules;
    std::string ratelimit_revision;
};

struct circuitbreaker_source {
    std::string service;
    std::string service_namespace;
    std::map<std::string, meta_label> meta_labels;
};

struct recover_config {
    std::string sleep_window;
    std::vector<int> request_rate_after_halfopen;
};

struct circuitbreaker_policy {
    struct error_rate_config {};
    struct error_rate_config error_rate;
    struct slow_rate_config {};
    struct slow_rate_config slow_rate;
};

struct circuitbreaker_destination {
    std::string service;
    std::string service_namespace;
    std::map<std::string, meta_label> meta_labels;
    int resource;
    int type;
    int scope;
    int metric_precision;
    std::string metric_window;
    std::string update_interval;
    struct recover_config recover;
    struct circuitbreaker_policy policy;
};

struct circuitbreaker_rule {
    std::vector<struct circuitbreaker_source> circuitbreaker_sources;
    std::vector<struct circuitbreaker_destination> circuitbreaker_destinations;
};

struct circuitbreaker {
    std::string id;
    std::string version;
    std::string circuitbreaker_name;
    std::string circuitbreaker_namespace;
    std::string service_name;
    std::string service_namespace;
    std::vector<struct circuitbreaker_rule> circuitbreaker_inbounds;
    std::vector<struct circuitbreaker_rule> circuitbreaker_outbounds;
    std::string revision;
};

struct circuitbreaker_request {
    int type;
    std::string service_name;
    std::string service_namespace;
    std::string revision;
};

struct circuitbreaker_result {
    int code;
    std::string info;
    std::string type;
    std::string service_name;
    std::string service_namespace;
    std::string service_revision;
    struct circuitbreaker data;
    std::string revision;
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

    void set_rule_based_router(bool router) { this->ptr->rule_based_router = router; }
    bool get_rule_based_router() const { return this->ptr->rule_based_router; }

    void set_nearby_based_router(bool router) { this->ptr->nearby_based_router = router; }
    bool get_nearby_based_router() const { return this->ptr->nearby_based_router; }

    void set_dst_meta_router(bool router) { this->ptr->dst_meta_router = router; }
    bool get_dst_meta_router() const { return this->ptr->dst_meta_router; }

    void set_nearby_match_level(const std::string level) { this->ptr->nearby_match_level = level; }
    const std::string get_nearby_match_level() const { return this->ptr->nearby_match_level; }

    void set_nearby_max_match_level(const std::string level) {
        this->ptr->nearby_max_match_level = level;
    }
    const std::string get_nearby_max_match_level() const {
        return this->ptr->nearby_max_match_level;
    }

    void set_nearby_unhealthy_degrade(bool degraded) {
        this->ptr->nearby_unhealthy_degrade = degraded;
    }
    bool get_nearby_unhealthy_degrade() const { return this->ptr->nearby_unhealthy_degrade; }

    void set_nearby_unhealthy_degrade_percent(int percent) {
        this->ptr->nearby_unhealthy_degrade_percent = percent;
    }
    int get_nearby_unhealthy_degrade_percent() const {
        return this->ptr->nearby_unhealthy_degrade_percent;
    }

    void set_circuit_breaker_enable(bool enable) { this->ptr->circuit_breaker_enable = enable; }
    bool get_circuit_breaker_enable() const { return this->ptr->circuit_breaker_enable; }

    void set_circuit_breaker_check_period(const std::string period) {
        this->ptr->circuit_breaker_check_period = period;
    }
    const std::string get_circuit_breaker_check_period() const {
        return this->ptr->circuit_breaker_check_period;
    }

    void set_circuit_breaker_error_count(bool enable) {
        this->ptr->circuit_breaker_error_count = enable;
    }
    bool get_circuit_breaker_error_count() const { return this->ptr->circuit_breaker_error_count; }

    void set_error_count_request_threshold(int threshold) {
        this->ptr->error_count_request_threshold = threshold;
    }
    int get_error_count_request_threshold() { return this->ptr->error_count_request_threshold; }

    void set_error_count_stat_time_window(const std::string window) {
        this->ptr->error_count_stat_time_window = window;
    }
    const std::string get_error_count_stat_time_window() const {
        return this->ptr->error_count_stat_time_window;
    }

    void set_error_count_sleep_window(const std::string window) {
        this->ptr->error_count_sleep_window = window;
    }
    const std::string get_error_count_sleep_window() const {
        return this->ptr->error_count_sleep_window;
    }

    void set_error_count_max_request_halfopen(int requests) {
        this->ptr->error_count_max_request_halfopen = requests;
    }
    int get_error_count_max_request_halfopen() const {
        return this->ptr->error_count_max_request_halfopen;
    }

    void set_error_count_least_success_halfopen(int requests) {
        this->ptr->error_count_least_success_halfopen = requests;
    }
    int get_error_count_least_success_halfopen() const {
        return this->ptr->error_count_least_success_halfopen;
    }

    void set_circuit_breaker_error_rate(bool enable) {
        this->ptr->circuit_breaker_error_rate = enable;
    }
    bool get_circuit_breaker_error_rate() const { return this->ptr->circuit_breaker_error_rate; }

    void set_error_rate_request_threshold(int threshold) const {
        this->ptr->error_rate_request_threshold = threshold;
    }
    int get_error_rate_request_threshold() { return this->ptr->error_rate_request_threshold; }

    void set_error_rate_threshold(double rate_threshold) {
        this->ptr->error_rate_threshold = rate_threshold;
    }
    double get_error_rate_threshold() const { return this->ptr->error_rate_threshold; }

    void set_error_rate_stat_time_window(const std::string window) {
        this->ptr->error_rate_stat_time_window = window;
    }
    const std::string get_error_rate_stat_time_window() const {
        return this->ptr->error_rate_stat_time_window;
    }

    void set_error_rate_num_buckets(int buckets) { this->ptr->error_rate_num_buckets = buckets; }
    int get_error_rate_num_buckets() const { return this->ptr->error_rate_num_buckets; }

    void set_error_rate_sleep_window(const std::string window) {
        this->ptr->error_rate_sleep_window = window;
    }
    const std::string get_error_rate_sleep_window() const {
        return this->ptr->error_rate_sleep_window;
    }

    void set_error_rate_max_request_halfopen(int requests) {
        this->ptr->error_rate_max_request_halfopen = requests;
    }
    int get_error_rate_max_request_halfopen() const {
        return this->ptr->error_rate_max_request_halfopen;
    }

    void set_error_rate_least_success_halfopen(int requests) {
        this->ptr->error_rate_least_success_halfopen = requests;
    }
    int get_error_rate_least_success_halfopen() const {
        return this->ptr->error_rate_least_success_halfopen;
    }

	int get_discover_refresh_seconds() const {
		return this->ptr->discover_refresh_seconds;
	}

    const struct polaris_config *get_polaris_config() const { return this->ptr; }

    void polaris_config_init() {
        polaris_config_init_system();
        polaris_config_init_router();
        polaris_config_init_api();
        polaris_config_init_circuit_breaker();
    }

    void polaris_config_init_system() {
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
    }

    void polaris_config_init_router() {
        this->ptr->rule_based_router = true;
        this->ptr->nearby_based_router = true;
        this->ptr->nearby_match_level = "zone";
        this->ptr->nearby_max_match_level = "none";
        this->ptr->nearby_unhealthy_degrade = true;
        this->ptr->nearby_unhealthy_degrade_percent = 100;
        this->ptr->nearby_enable_recover_all = true;
        this->ptr->dst_meta_router = false;
    }

    void polaris_config_init_api() {
        this->ptr->api_bindIf = "eth0";
        this->ptr->api_bindIP = "127.0.0.1";
        this->ptr->api_timeout_seconds = 1;
        this->ptr->api_retry_max = 3;
        this->ptr->api_retry_seconds = 1;
        this->ptr->state_report_enable = false;
    }
    void polaris_config_init_circuit_breaker() {
        this->ptr->circuit_breaker_enable = true;
        this->ptr->circuit_breaker_check_period = "500ms";
        this->ptr->circuit_breaker_error_count = true;
        this->ptr->error_count_request_threshold = 10;
        this->ptr->error_count_stat_time_window = "1m";
        this->ptr->error_count_sleep_window = "5s";
        this->ptr->error_count_max_request_halfopen = 3;
        this->ptr->error_count_least_success_halfopen = 2;
        this->ptr->circuit_breaker_error_rate = true;
        this->ptr->error_rate_request_threshold = 10;
        this->ptr->error_rate_threshold = 0.5;
        this->ptr->error_rate_stat_time_window = "1m";
        this->ptr->error_rate_num_buckets = 12;
        this->ptr->error_rate_sleep_window = "3s";
        this->ptr->error_rate_max_request_halfopen = 3;
        this->ptr->error_rate_least_success_halfopen = 2;
    }

  private:
    std::atomic<int> *ref;
    struct polaris_config *ptr;
};

};  // namespace polaris

#endif
