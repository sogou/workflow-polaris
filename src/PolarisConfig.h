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
    // polaris global config
    // global/system
    std::string discover_namespace;
    std::string discover_name;
    uint64_t discover_refresh_interval;
    std::string healthcheck_namespace;
    std::string healthcheck_name;
    uint64_t healthcheck_refresh_interval;
    std::string monitor_namespace;
    std::string monitor_name;
    uint64_t monitor_refresh_interval;
    std::string metric_namespace;
    std::string metric_name;
    uint64_t metric_refresh_interval;
    // global/api
    std::string api_bindIf;
    std::string api_bindIP;
    std::string api_location_zone;
    std::string api_location_region;
    std::string api_location_campus;
    uint64_t api_timeout_milliseconds;
    int api_retry_max;
    uint64_t api_retry_milliseconds;
    // global/serverConnector  Deprecated, use client url
    // polaris server address
    //std::vector<std::string> server_connector_hosts;
    // polaris server protocol: http, grpc, trpc
    //std::string server_connector_protocol;
    // connect timeout eg. 100ms
    //uint64_t server_connect_timeout;
    // global/stateReport's config
    bool state_report_enable;
    std::vector<std::string> state_report_chain;
    uint64_t state_report_window;
    int state_report_buckets;

    // polaris consumer config
    // consumer/localCache
    // 服务定期刷新周期
    uint64_t service_refresh_interval;
    uint64_t service_expire_time;
    // consumer/circuitBreaker: 熔断
    // 是否启用节点熔断功能
    bool circuit_breaker_enable;
    // 实例定时熔断检测周期
    uint64_t circuit_breaker_check_period;
    // 熔断插件名
    std::vector<std::string> circuit_breaker_chain;
    // circuitBreaker plugin: errorCount
    // 是否启用基于周期连续错误数熔断策略配置
    // 触发连续错误熔断的阈值
    int error_count_request_threshold;
    // 连续失败的统计周期
    uint64_t error_count_stat_time_window;
    // 熔断器打开后，多久后转换为半开状态
    uint64_t error_count_sleep_window;
    // 熔断器半开后最大允许的请求数
    int error_count_max_request_halfopen;
    // 熔断器半开到关闭所必须的最少成功请求数
    int error_count_least_success_halfopen;
    // circuitBreaker plugin: errorRate
    // 基于周期错误率的熔断策略配置
    // 触发错误率熔断的最低请求阈值
    int error_rate_request_threshold;
    // 触发错误率熔断的阈值
    double error_rate_threshold;
    // 错误率熔断的统计周期
    uint64_t error_rate_stat_time_window;
    // 错误率熔断的最小统计单元数量
    int error_rate_num_buckets;
    // 熔断器打开后，多久后转换为半开状态
    uint64_t error_rate_sleep_window;
    // 熔断器半开后最大允许的请求数
    int error_rate_max_request_halfopen;
    // 熔断器半开到关闭所必须的最少成功请求数
    int error_rate_least_success_halfopen;
    // set集群熔断开关
    bool setcluster_circuit_breaker_enable;
    // consumer/healthCheck: 故障探测
    // 是否启用故障探测功能
    bool health_check_enable;
    // 定时故障探测周期
    uint64_t health_check_period;
    // 故障探测策略
    std::vector<std::string> health_check_chain;
    // 基于TCP的故障探测策略
    // 探测超时时间
    uint64_t plugin_tcp_timeout;
    // 探测失败重试次数
    int plugin_tcp_retry;
    // tcp发送的探测包，可选字段，假如不配置，则默认只做连接探测
    std::string plugin_tcp_send;
    // 期望接收的TCP回复包，可选字段，假如不配置，则默认只做连接或发包探测
    std::string plugin_tcp_receive;
    // 基于UDP的故障探测策略
    // UDP探测超时时间
    uint64_t plugin_udp_timeout;
    // UDP探测失败重试次数
    int plugin_udp_retry;
    // udp发送的探测包，必选字段，假如不配置，则不启动UDP探测
    std::string plugin_udp_send;
    // 期望接收的UDP回复包，必选字段，假如不配置，则不启动UDP探测
    std::string plugin_udp_receive;
    // 基于HTTP的故障探测策略
    // HTTP探测超时时间
    uint64_t plugin_http_timeout;
    // http探测路径，必选字段，假如不配置，则不启用http探测
    std::string plugin_http_path;
    std::string load_balancer_type;
    // consumer/router: 路由
    // 基于主调和被调服务规则的路由策略
    // 就近路由策略
    std::vector<std::string> service_router_chain;
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

    // polaris rateLimiter config
    std::string rate_limit_mode;
    std::string rate_limit_cluster_namespace;
    std::string rate_limit_cluster_name;
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

	PolarisInstance(const PolarisInstance &copy) {
		this->~PolarisInstance();
		this->ref = copy.ref;
		this->inst = copy.inst;
		++*this->ref;
	}

    PolarisInstance(PolarisInstance &&move) {
        this->ref = move.ref;
        this->inst = move.inst;
        move.ref = new std::atomic<int>(1);
        move.inst = new instance;
        move.instance_init();
    }

    PolarisInstance &operator=(const PolarisInstance &copy) {
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
        this->inst->healthcheck_type = "HEARTBEAT";
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

    int init_from_yaml(const std::string &yaml_file);

    std::string get_discover_namespace() const { return this->ptr->discover_namespace; }
    std::string get_discover_name() const { return this->ptr->discover_name; }
    uint64_t get_discover_refresh_interval() const { return this->ptr->discover_refresh_interval; }
    std::string get_healthcheck_namespace() const { return this->ptr->healthcheck_namespace; }
    std::string get_healthcheck_name() const { return this->ptr->healthcheck_name; }
    uint64_t get_healthcheck_refresh_interval() const {
        return this->ptr->healthcheck_refresh_interval;
    }
    std::string get_monitor_namespace() const { return this->ptr->monitor_namespace; }
    std::string get_monitor_name() const { return this->ptr->monitor_name; }
    uint64_t get_monitor_refresh_interval() const { return this->ptr->monitor_refresh_interval; }
    std::string get_metric_namespace() const { return this->ptr->metric_namespace; }
    std::string get_metric_name() const { return this->ptr->metric_name; }
    uint64_t get_metric_refresh_interval() const { return this->ptr->metric_refresh_interval; }
    std::string get_api_bindIf() const { return this->ptr->api_bindIf; }
    std::string get_api_bindIP() const { return this->ptr->api_bindIP; }
    std::string get_api_location_zone() const { return this->ptr->api_location_zone; }
    std::string get_api_location_region() const { return this->ptr->api_location_region; }
    std::string get_api_location_campus() const { return this->ptr->api_location_campus; }
    uint64_t get_api_timeout_milliseconds() const { return this->ptr->api_timeout_milliseconds; }
    int get_api_retry_max() const { return this->ptr->api_retry_max; }
    uint64_t get_api_retry_milliseconds() const { return this->ptr->api_retry_milliseconds; }
    bool get_state_report_enable() const { return this->ptr->state_report_enable; }
    std::vector<std::string> get_state_report_chain() const {
        return this->ptr->state_report_chain;
    }
    uint64_t get_state_report_window() const { return this->ptr->state_report_window; }
    int get_state_report_buckets() const { return this->ptr->state_report_buckets; }
    uint64_t get_service_refresh_interval() const { return this->ptr->service_refresh_interval; }
    uint64_t get_service_expire_time() const { return this->ptr->service_expire_time; }
    bool get_circuit_breaker_enable() const { return this->ptr->circuit_breaker_enable; }
    uint64_t get_circuit_breaker_check_period() const {
        return this->ptr->circuit_breaker_check_period;
    }
    std::vector<std::string> get_circuit_breaker_chain() const {
        return this->ptr->circuit_breaker_chain;
    }
    int get_error_count_request_threshold() const {
        return this->ptr->error_count_request_threshold;
    }
    uint64_t get_error_count_stat_time_window() const {
        return this->ptr->error_count_stat_time_window;
    }
    uint64_t get_error_count_sleep_window() const { return this->ptr->error_count_sleep_window; }
    int get_error_count_max_request_halfopen() const {
        return this->ptr->error_count_max_request_halfopen;
    }
    int get_error_count_least_success_halfopen() const {
        return this->ptr->error_count_least_success_halfopen;
    }
    int get_error_rate_request_threshold() const { return this->ptr->error_rate_request_threshold; }
    double get_error_rate_threshold() const { return this->ptr->error_rate_threshold; }
    uint64_t get_error_rate_stat_time_window() const {
        return this->ptr->error_rate_stat_time_window;
    }
    int get_error_rate_num_buckets() const { return this->ptr->error_rate_num_buckets; }
    uint64_t get_error_rate_sleep_window() const { return this->ptr->error_rate_sleep_window; }
    int get_error_rate_max_request_halfopen() const {
        return this->ptr->error_rate_max_request_halfopen;
    }
    int get_error_rate_least_success_halfopen() const {
        return this->ptr->error_rate_least_success_halfopen;
    }

    bool get_setcluster_circuit_breaker_enable() const {
        return this->ptr->setcluster_circuit_breaker_enable;
    }
    bool get_health_check_enable() const { return this->ptr->health_check_enable; }
    uint64_t get_health_check_period() const { return this->ptr->health_check_period; }
    std::vector<std::string> get_health_check_chain() const {
        return this->ptr->health_check_chain;
    }
    uint64_t get_plugin_tcp_timeout() const { return this->ptr->plugin_tcp_timeout; }
    int get_plugin_tcp_retry() const { return this->ptr->plugin_tcp_retry; }
    std::string get_plugin_tcp_send() const { return this->ptr->plugin_tcp_send; }
    std::string get_plugin_tcp_receive() const { return this->ptr->plugin_tcp_receive; }
    uint64_t get_plugin_udp_timeout() const { return this->ptr->plugin_udp_timeout; }
    int get_plugin_udp_retry() const { return this->ptr->plugin_udp_retry; }
    std::string get_plugin_udp_send() const { return this->ptr->plugin_udp_send; }
    std::string get_plugin_udp_receive() const { return this->ptr->plugin_udp_receive; }
    uint64_t get_plugin_http_timeout() const { return this->ptr->plugin_http_timeout; }
    std::string get_plugin_http_path() const { return this->ptr->plugin_http_path; }
    std::string get_load_balancer_type() const { return this->ptr->load_balancer_type; }
    std::vector<std::string> get_service_router_chain() const {
        return this->ptr->service_router_chain;
    }
    std::string get_nearby_match_level() const { return this->ptr->nearby_match_level; }
    const std::string get_nearby_max_match_level() const {
        return this->ptr->nearby_max_match_level;
    }
    bool get_nearby_unhealthy_degrade() const { return this->ptr->nearby_unhealthy_degrade; }
    int get_nearby_unhealthy_degrade_percent() const {
        return this->ptr->nearby_unhealthy_degrade_percent;
    }
    bool get_nearby_enable_recover_all() const { return this->ptr->nearby_enable_recover_all; }
    std::string get_rate_limit_mode() const { return this->ptr->rate_limit_mode; }
    std::string get_rate_limit_cluster_namespace() const {
        return this->ptr->rate_limit_cluster_namespace;
    }
    std::string get_rate_limit_cluster_name() const { return this->ptr->rate_limit_cluster_name; }
    // get all polaris_config
    const struct polaris_config *get_polaris_config() const { return this->ptr; }

    void polaris_config_init() {
        polaris_config_init_global();
        polaris_config_init_consumer();
        polaris_config_init_ratelimiter();
    }

    void polaris_config_init_global() {
        this->ptr->discover_namespace = DEFAULT_NAMESPACE;
        this->ptr->discover_name = "polaris.discover";
        this->ptr->discover_refresh_interval = 6000000;  // 10 * 60 * 1000 milliseconds
        this->ptr->healthcheck_namespace = DEFAULT_NAMESPACE;
        this->ptr->healthcheck_name = "polaris.healthcheck";
        this->ptr->healthcheck_refresh_interval = 6000000;
        this->ptr->monitor_namespace = DEFAULT_NAMESPACE;
        this->ptr->monitor_name = "polaris.monitor";
        this->ptr->monitor_refresh_interval = 6000000;
        this->ptr->metric_namespace = DEFAULT_NAMESPACE;
        this->ptr->metric_name = "polaris.metric";
        this->ptr->metric_refresh_interval = 6000000;
        this->ptr->api_bindIf = "eth0";
        this->ptr->api_bindIP = "127.0.0.1";
        this->ptr->api_location_zone = "unknown";
        this->ptr->api_location_region = "unknown";
        this->ptr->api_location_campus = "unknown";
        this->ptr->api_timeout_milliseconds = 1000;
        this->ptr->api_retry_max = 3;
        this->ptr->api_retry_milliseconds = 1000;
        this->ptr->state_report_enable = false;
        this->ptr->state_report_chain.push_back("stat2Monitor");
        this->ptr->state_report_window = 60000;
        this->ptr->state_report_buckets = 12;
    }

    void polaris_config_init_consumer() {
        this->ptr->service_refresh_interval = 2000;
        this->ptr->service_expire_time = 86400000;
        this->ptr->circuit_breaker_enable = true;
        this->ptr->circuit_breaker_check_period = 500;
        this->ptr->circuit_breaker_chain.push_back("errorCount");
        this->ptr->circuit_breaker_chain.push_back("errorRate");
        this->ptr->error_count_request_threshold = 10;
        this->ptr->error_count_stat_time_window = 60000;
        this->ptr->error_count_sleep_window = 5000;
        this->ptr->error_count_max_request_halfopen = 3;
        this->ptr->error_count_least_success_halfopen = 2;
        this->ptr->error_rate_request_threshold = 10;
        this->ptr->error_rate_threshold = 0.5;
        this->ptr->error_rate_stat_time_window = 60000;
        this->ptr->error_rate_num_buckets = 12;
        this->ptr->error_rate_sleep_window = 3000;
        this->ptr->error_rate_max_request_halfopen = 3;
        this->ptr->error_rate_least_success_halfopen = 2;
        this->ptr->health_check_enable = true;
        this->ptr->health_check_period = 60000;
        this->ptr->health_check_chain.push_back("tcp");
        this->ptr->plugin_tcp_timeout = 100;
        this->ptr->plugin_tcp_retry = 0;
        this->ptr->plugin_tcp_send = "";
        this->ptr->plugin_tcp_receive = "";
        this->ptr->plugin_udp_timeout = 100;
        this->ptr->plugin_udp_retry = 0;
        this->ptr->plugin_udp_send = "";
        this->ptr->plugin_udp_receive = "";
        this->ptr->plugin_http_timeout = 100;
        this->ptr->plugin_http_path = "/ping";
        this->ptr->load_balancer_type = "weightedRandom";
        this->ptr->service_router_chain.push_back("ruleBasedRouter");
        this->ptr->service_router_chain.push_back("nearbyBasedRouter");
        this->ptr->nearby_match_level = "zone";
        this->ptr->nearby_max_match_level = "none";
        this->ptr->nearby_unhealthy_degrade = true;
        this->ptr->nearby_unhealthy_degrade_percent = 100;
        this->ptr->nearby_enable_recover_all = true;
    }

    void polaris_config_init_ratelimiter() {
        this->ptr->rate_limit_mode = "local";
        this->ptr->rate_limit_cluster_namespace = "Polaris";
        this->ptr->rate_limit_cluster_name = "polaris.metric";
    }

  private:
    std::atomic<int> *ref;
    struct polaris_config *ptr;
};

};  // namespace polaris

#endif
