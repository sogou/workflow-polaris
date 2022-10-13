#include "PolarisConfig.h"
#include "json.hpp"
#include "yaml-cpp/yaml.h"

using nlohmann::json;

namespace polaris {

static const int kDefaultInstancePort = 80;
static const int kDefaultInstancePriority = 0;
static const int kDefaultInstanceWeight = 100;
static const int kDefaultInstanceHealthCheckTTL = 5;
static const int kDefaultInstanceHealthCheckType = 1;

static const std::string kDefaultMetaMatchType = "EXACT";
static const std::string kDefaultMetaValueType = "TEXT";

void to_json(json &j, const struct discover_request &request) {
    j = json{{"type", request.type},
             {"service",
              {{"name", request.service_name},
               {"namespace", request.service_namespace},
               {"revision", request.revision}}}};
}

void to_json(json &j, const struct register_request &request) {
    j = json{{"service", request.service},
             {"namespace", request.service_namespace},
             {"host", request.inst.host},
             {"port", request.inst.port},
             {"enable_health_check", request.inst.enable_healthcheck},
             {"healthy", request.inst.healthy},
             {"isolate", request.inst.isolate},
             {"weight", request.inst.weight}};
    if (!request.service_token.empty()) {
        j["service_token"] = request.service_token;
    }
    if (!request.inst.protocol.empty()) {
        j["protocol"] = request.inst.protocol;
    }
    if (!request.inst.version.empty()) {
        j["version"] = request.inst.version;
    }
    if (request.inst.enable_healthcheck == true) {
        j["health_check"]["type"] = request.inst.healthcheck_type;
        j["health_check"]["heartbeat"]["ttl"] = request.inst.healthcheck_ttl;
    }
    if (!request.inst.region.empty()) {
        j["location"]["region"] = request.inst.region;
    }
    if (!request.inst.zone.empty()) {
        j["location"]["zone"] = request.inst.zone;
    }
    if (!request.inst.campus.empty()) {
        j["location"]["campus"] = request.inst.campus;
    }
    if (!request.inst.logic_set.empty()) {
        j["logic_set"] = request.inst.logic_set;
    }
    if (!request.inst.metadata.empty()) {
        auto iter = request.inst.metadata.begin();
        for (; iter != request.inst.metadata.end(); iter++) {
            j["metadata"][iter->first] = iter->second;
        }
    }
}

void to_json(json &j, const struct deregister_request &request) {
    if (!request.id.empty()) {
        j = json{{"id", request.id}};
    } else {
        j = json{{"service", request.service},
                 {"namespace", request.service_namespace},
                 {"host", request.host},
                 {"port", request.port}};
    }
    if (!request.service_token.empty()) {
        j["service_token"] = request.service_token;
    }
}

void to_json(json &j, const struct ratelimit_request &request) {
    j = json{{"type", request.type},
             {"service",
              {{"name", request.service_name},
               {"namespace", request.service_namespace},
               {"revision", request.revision}}}};
}

void to_json(json &j, const struct circuitbreaker_request &request) {
    j = json{{"type", request.type},
             {"service",
              {{"name", request.service_name},
               {"namespace", request.service_namespace},
               {"revision", request.revision}}}};
}

void from_json(const json &j, struct instance &response) {
    response.priority = j.value("priority", kDefaultInstancePriority);
    response.port = j.value("port", kDefaultInstancePort);
    response.weight = j.value("weight", kDefaultInstanceWeight);
    response.enable_healthcheck = j.value("enableHealthCheck", false);
    response.healthy = j.value("healthy", true);
    response.isolate = j.value("isolate", false);
    if (j.find("healthCheck") != j.end()) {
        if (j.at("healthCheck").find("type") != j.at("healthCheck").end()) {
            j.at("healthCheck").at("type").get_to(response.healthcheck_type);
        } else {
            response.healthcheck_type = kDefaultInstanceHealthCheckType;
        }
        if (j.at("healthCheck").find("heartbeat") != j.at("healthCheck").end()) {
            if (j.at("healthCheck").at("heartbeat").find("ttl") !=
                j.at("healthCheck").at("heartbeat").end()) {
                j.at("healthCheck").at("heartbeat").at("ttl").get_to(response.healthcheck_ttl);
            } else {
                response.healthcheck_ttl = kDefaultInstanceHealthCheckTTL;
            }
        }
    }
    j.at("id").get_to(response.id);
    j.at("service").get_to(response.service);
    j.at("namespace").get_to(response.service_namespace);
    j.at("vpc_id").get_to(response.vpc_id);
    j.at("host").get_to(response.host);
    j.at("protocol").get_to(response.protocol);
    j.at("version").get_to(response.version);
    if (j.find("metadata") != j.end()) {
        j.at("metadata").get_to<std::map<std::string, std::string>>(response.metadata);
    }
    j.at("logic_set").get_to(response.logic_set);
    j.at("mtime").get_to(response.mtime);
    j.at("revision").get_to(response.revision);
}

void from_json(const json &j, struct discover_result &response) {
    int code = j.at("code").get<int>();
    switch (code) {
        case 200001:
            j.at("code").get_to(response.code);
            j.at("info").get_to(response.info);
            j.at("type").get_to(response.type);
            j.at("service").at("namespace").get_to(response.service_namespace);
            j.at("service").at("name").get_to(response.service_name);
            j.at("service").at("revision").get_to(response.service_revision);
            break;
        case 200000:
            j.at("code").get_to(response.code);
            j.at("info").get_to(response.info);
            j.at("type").get_to(response.type);
            j.at("service").at("namespace").get_to(response.service_namespace);
            j.at("service").at("name").get_to(response.service_name);
            j.at("service").at("revision").get_to(response.service_revision);
            response.service_metadata.clear();
            j.at("service")
                .at("metadata")
                .get_to<std::map<std::string, std::string>>(response.service_metadata);
            j.at("service").at("ports").get_to(response.service_ports);
            j.at("service").at("business").get_to(response.service_business);
            j.at("service").at("department").get_to(response.service_department);
            j.at("service").at("cmdb_mod1").get_to(response.service_cmdbmod1);
            j.at("service").at("cmdb_mod2").get_to(response.service_cmdbmod2);
            j.at("service").at("cmdb_mod3").get_to(response.service_cmdbmod3);
            j.at("service").at("comment").get_to(response.service_comment);
            j.at("service").at("owners").get_to(response.service_owners);
            j.at("service").at("ctime").get_to(response.service_ctime);
            j.at("service").at("mtime").get_to(response.service_mtime);
            j.at("service").at("platform_id").get_to(response.service_platform_id);
            response.instances.clear();
            j.at("instances").get_to<std::vector<struct instance>>(response.instances);
            break;
        default:
            break;
    }
}

void from_json(const json &j, struct meta_label &response) {
    response.type = j.value("type", kDefaultMetaMatchType);
    response.value_type = j.value("valueType", kDefaultMetaValueType);
    j.at("value").get_to(response.value);
}

void from_json(const json &j, struct source_bound &response) {
    j.at("service").get_to(response.service);
    j.at("namespace").get_to(response.service_namespace);
    response.metadata.clear();
    j.at("metadata").get_to<std::map<std::string, struct meta_label>>(response.metadata);
}

void from_json(const json &j, struct destination_bound &response) {
    j.at("service").get_to(response.service);
    j.at("namespace").get_to(response.service_namespace);
    response.metadata.clear();
    j.at("metadata").get_to<std::map<std::string, struct meta_label>>(response.metadata);
    response.priority = j.value("priority", kDefaultInstancePriority);
    response.weight = j.value("weight", kDefaultInstanceWeight);
}

void from_json(const json &j, struct routing_bound &response) {
    response.source_bounds.clear();
    response.destination_bounds.clear();
    j.at("sources").get_to<std::vector<struct source_bound>>(response.source_bounds);
    j.at("destinations").get_to<std::vector<struct destination_bound>>(response.destination_bounds);
}

void from_json(const json &j, struct route_result &response) {
    int code = j.at("code").get<int>();
    switch (code) {
        case 200001:
            j.at("code").get_to(response.code);
            j.at("info").get_to(response.info);
            j.at("type").get_to(response.type);
            j.at("service").at("name").get_to(response.service_name);
            j.at("service").at("namespace").get_to(response.service_namespace);
            break;
        case 200000:
            j.at("code").get_to(response.code);
            j.at("info").get_to(response.info);
            j.at("type").get_to(response.type);
            j.at("service").at("name").get_to(response.service_name);
            j.at("service").at("namespace").get_to(response.service_namespace);
            if (j.find("routing") != j.end()) {
                j.at("routing").at("service").get_to(response.routing_service);
                j.at("routing").at("namespace").get_to(response.routing_namespace);
                response.routing_inbounds.clear();
                j.at("routing")
                    .at("inbounds")
                    .get_to<std::vector<struct routing_bound>>(response.routing_inbounds);
                response.routing_outbounds.clear();
                j.at("routing")
                    .at("outbounds")
                    .get_to<std::vector<struct routing_bound>>(response.routing_outbounds);
                j.at("routing").at("ctime").get_to(response.routing_ctime);
                j.at("routing").at("mtime").get_to(response.routing_mtime);
                j.at("routing").at("revision").get_to(response.routing_revision);
            }
            break;
        default:
            break;
    }
}

void from_json(const json &j, struct ratelimit_amount &response) {
    j.at("maxAmount").get_to(response.max_amount);
    j.at("validDuration").get_to(response.valid_duration);
}

void from_json(const json &j, struct ratelimit_rule &response) {
    j.at("id").get_to(response.id);
    j.at("service").get_to(response.service);
    j.at("namespace").get_to(response.service_namespace);
    j.at("priority").get_to(response.priority);
    j.at("type").get_to(response.type);
    j.at("labels").get_to<std::map<std::string, struct meta_label>>(response.meta_labels);
    j.at("amounts").get_to<std::vector<struct ratelimit_amount>>(response.ratelimit_amounts);
    j.at("action").get_to(response.action);
    j.at("disable").get_to(response.disable);
    j.at("ctime").get_to(response.ctime);
    j.at("mtime").get_to(response.mtime);
    j.at("revision").get_to(response.revision);
}

void from_json(const json &j, struct ratelimit_result &response) {
    int code = j.at("code").get<int>();
    switch (code) {
        case 200000:
            j.at("code").get_to(response.code);
            j.at("info").get_to(response.info);
            j.at("type").get_to(response.type);
            j.at("service").at("name").get_to(response.service_name);
            j.at("service").at("namespace").get_to(response.service_namespace);
            if (j.at("service").find("revision") != j.at("service").end()) {
                j.at("service").at("revision").get_to(response.service_revision);
            }
            if (j.find("ratelimit") != j.end()) {
                j.at("ratelimit")
                    .at("rules")
                    .get_to<std::vector<struct ratelimit_rule>>(response.ratelimit_rules);
                j.at("ratelimit").at("revision").get_to(response.ratelimit_revision);
            }
            break;
        default:
            break;
    }
}

void from_json(const json &j, struct circuitbreaker_source &response) {
    j.at("service").get_to(response.service);
    j.at("namespace").get_to(response.service_namespace);
    j.at("labels").get_to<std::map<std::string, struct meta_label>>(response.meta_labels);
}

void from_json(const json &j, struct circuitbreaker_destination &response) {
    j.at("service").get_to(response.service);
    j.at("namespace").get_to(response.service_namespace);
    j.at("labels").get_to<std::map<std::string, struct meta_label>>(response.meta_labels);
    j.at("metricWindow").get_to(response.metric_window);
    j.at("metricPrecision").get_to(response.metric_precision);
    j.at("updateInterval").get_to(response.update_interval);
    // todo: convert recover and circuitbreaker_policy
}

void from_json(const json &j, struct circuitbreaker_rule &response) {
    j.at("sources").get_to<std::vector<struct circuitbreaker_source>>(
        response.circuitbreaker_sources);
    j.at("destinations")
        .get_to<std::vector<struct circuitbreaker_destination>>(
            response.circuitbreaker_destinations);
}

void from_json(const json &j, struct circuitbreaker &response) {
    j.at("id").get_to(response.id);
    j.at("version").get_to(response.id);
    j.at("name").get_to(response.circuitbreaker_name);
    j.at("namespace").get_to(response.circuitbreaker_namespace);
    j.at("service").get_to(response.service_name);
    j.at("service_namespace").get_to(response.service_namespace);
    j.at("inbounds")
        .get_to<std::vector<struct circuitbreaker_rule>>(response.circuitbreaker_inbounds);
    j.at("outbounds")
        .get_to<std::vector<struct circuitbreaker_rule>>(response.circuitbreaker_outbounds);
    j.at("revision").get_to(response.revision);
}

void from_json(const json &j, struct circuitbreaker_result &response) {
    int code = j.at("code").get<int>();
    switch (code) {
        case 200000:
            j.at("code").get_to(response.code);
            j.at("info").get_to(response.info);
            j.at("type").get_to(response.type);
            j.at("service").at("name").get_to(response.service_name);
            j.at("service").at("namespace").get_to(response.service_namespace);
            if (j.at("service").find("revision") != j.at("service").end()) {
                j.at("service").at("revision").get_to(response.service_revision);
            }
            if (j.find("circuitBreaker") != j.end()) {
                j.at("circuitBreaker").get_to(response.data);
            }
            break;
        default:
            break;
    }
}

bool ParseTimeValue(std::string &time_value, uint64_t &result) {
    uint64_t base = 1;
    if (time_value.length() >= 2) {
        if (time_value[time_value.length() - 1] == 'h') {  // hour
            time_value = time_value.substr(0, time_value.length() - 1);
            base = 60 * 60 * 1000;
        } else if (time_value[time_value.length() - 1] == 'm') {  // minute
            time_value = time_value.substr(0, time_value.length() - 1);
            base = 60 * 1000;
        } else if (time_value[time_value.length() - 1] == 's') {
            if (time_value[time_value.length() - 2] == 'm') {  // millsecond
                time_value = time_value.substr(0, time_value.length() - 2);
            } else {  // second
                time_value = time_value.substr(0, time_value.length() - 1);
                base = 1000;
            }
        }
    }
    result = 0;
    for (std::size_t i = 0; i < time_value.size(); ++i) {
        if (isdigit(time_value[i])) {
            result = result * 10 + (time_value[i] - '0');
        } else {
            return false;
        }
    }
    result = result * base;
    return true;
}

int init_global_from_yaml(struct polaris_config *ptr, const YAML::Node &node) {
    // init global config
    if (!node["global"].IsDefined()) {
        return 0;
    }
    YAML::Node global = node["global"];
    // init system
    if (global["system"].IsDefined() && !global["system"].IsNull()) {
        YAML::Node system = global["system"];
        // init system's discoverCluster
        if (system["discoverCluster"].IsDefined() && !system["discoverCluster"].IsNull()) {
            YAML::Node discover = system["discoverCluster"];
            ptr->discover_namespace = discover["namespace"].as<std::string>("Polaris");
            ptr->discover_name = discover["service"].as<std::string>("polaris.discover");
            std::string discover_interval = discover["refreshInterval"].as<std::string>("10m");
            uint64_t discover_interval_ms;
            if (!ParseTimeValue(discover_interval, discover_interval_ms)) {
                return -1;
            }
            ptr->discover_refresh_interval = discover_interval_ms;
        }
        // init system's helathCluster
        if (system["healthCheckCluster"].IsDefined() && !system["healthCheckCluster"].IsNull()) {
            YAML::Node health = system["healthCheckCluster"];
            ptr->healthcheck_namespace = health["namespace"].as<std::string>("Polaris");
            ptr->healthcheck_name = health["service"].as<std::string>("polaris.healthcheck");
            std::string health_interval = health["refreshInterval"].as<std::string>("10m");
            uint64_t health_interval_ms;
            if (!ParseTimeValue(health_interval, health_interval_ms)) {
                return -1;
            }
            ptr->healthcheck_refresh_interval = health_interval_ms;
        }
        // init system's monitorCluster
        if (system["monitorCluster"].IsDefined() && !system["monitorCluster"].IsNull()) {
            YAML::Node monitor = system["monitorCluster"];
            ptr->monitor_namespace = monitor["namespace"].as<std::string>("Polaris");
            ptr->monitor_name = monitor["service"].as<std::string>("polaris.monitor");
            std::string monitor_interval = monitor["refreshInterval"].as<std::string>("10m");
            uint64_t monitor_interval_ms;
            if (!ParseTimeValue(monitor_interval, monitor_interval_ms)) {
                return -1;
            }
            ptr->monitor_refresh_interval = monitor_interval_ms;
        }
        // init system's metricCluster
        if (system["metricCluster"].IsDefined() && !system["metricCluster"].IsNull()) {
            YAML::Node metric = system["metricCluster"];
            ptr->metric_namespace = metric["namespace"].as<std::string>("Polaris");
            ptr->metric_name = metric["service"].as<std::string>("polaris.monitor");
            std::string metric_interval = metric["refreshInterval"].as<std::string>("10m");
            uint64_t metric_interval_ms;
            if (!ParseTimeValue(metric_interval, metric_interval_ms)) {
                return -1;
            }
            ptr->metric_refresh_interval = metric_interval_ms;
        }
    }

    // init api config
    if (global["api"].IsDefined() && !global["api"].IsNull()) {
        YAML::Node api = global["api"];
        ptr->api_bindIf = api["bindIf"].as<std::string>("eth1");
        ptr->api_bindIP = api["bindIP"].as<std::string>("127.0.0.1");
        std::string api_timeout = api["timeout"].as<std::string>("1s");
        uint64_t api_timeout_ms;
        if (!ParseTimeValue(api_timeout, api_timeout_ms)) {
            return -1;
        }
        ptr->api_timeout_milliseconds = api_timeout_ms;
        ptr->api_retry_max = api["maxRetryTimes"].as<int>(3);
        std::string api_retry_interval = api["retryInterval"].as<std::string>("1s");
        uint64_t api_retry_interval_ms;
        if (!ParseTimeValue(api_retry_interval, api_retry_interval_ms)) {
            return -1;
        }
        ptr->api_retry_milliseconds = api_retry_interval_ms;
        if (api["location"].IsDefined() && !api["location"].IsNull()) {
            YAML::Node location = api["location"];
            ptr->api_location_zone = location["zone"].as<std::string>("unknown");
            ptr->api_location_region = location["region"].as<std::string>("unknown");
            ptr->api_location_campus = location["campus"].as<std::string>("unknown");
        }
    }

    // init serverConnector config
	/* Deprecated, use client url
    if (global["serverConnector"].IsDefined() && !global["serverConnector"].IsNull()) {
        YAML::Node server_connector = global["serverConnector"];
        if (server_connector["addresses"].IsDefined()) {
            YAML::Node connector_hosts = server_connector["addresses"];
            if (connector_hosts.size() > 0) {
                ptr->server_connector_hosts.clear();
                for (YAML::const_iterator iter = connector_hosts.begin();
                     iter != connector_hosts.end(); ++iter) {
                    std::string host = iter->as<std::string>("127.0.0.1:8888");
                    ptr->server_connector_hosts.push_back(host);
                }
            }
        }
        ptr->server_connector_protocol = server_connector["protocol"].as<std::string>("http");
        std::string server_connect_timeout =
            server_connector["connectTimeout"].as<std::string>("200ms");
        uint64_t server_connect_timeout_ms;
        if (!ParseTimeValue(server_connect_timeout, server_connect_timeout_ms)) {
            return -1;
        }
        ptr->server_connect_timeout = server_connect_timeout_ms;
    }*/
    if (global["statReporter"].IsDefined() && !global["statReporter"].IsNull()) {
        YAML::Node state_report = global["statReporter"];
        ptr->state_report_enable = state_report["enable"].as<bool>(true);
        if (state_report["chain"].IsDefined()) {
            YAML::Node state_report_chain = state_report["chain"];
            if (state_report_chain.size() > 0) {
                ptr->state_report_chain.clear();
                for (YAML::const_iterator iter = state_report_chain.begin();
                     iter != state_report_chain.end(); ++iter) {
                    ptr->state_report_chain.push_back(iter->as<std::string>());
                }
            }
        }
        if (state_report["plugin"].IsDefined()) {
            if (state_report["plugin"]["stat2Monitor"].IsDefined()) {
                YAML::Node state_monitor = state_report["plugin"]["stat2Monitor"];
                std::string state_report_window =
                    state_monitor["metricsReportWindow"].as<std::string>("1m");
                uint64_t state_report_window_ms;
                if (!ParseTimeValue(state_report_window, state_report_window_ms)) {
                    return -1;
                }
                ptr->state_report_window = state_report_window_ms;
                ptr->state_report_buckets = state_monitor["metricsNumBuckets"].as<int>(12);
            }
        }
    }
    return 0;
}

int init_consumer_from_yaml(struct polaris_config *ptr, const YAML::Node &node) {
    // init consumer config
    if (!node["consumer"].IsDefined()) {
        return 0;
    }
    YAML::Node consumer = node["consumer"];
    // init service refresh interval
    if (consumer["localCache"].IsDefined()) {
        YAML::Node local_cache = consumer["localCache"];
        std::string service_refresh_interval =
            local_cache["serviceRefreshInterval"].as<std::string>("2s");
        uint64_t service_refresh_interval_ms;
        if (!ParseTimeValue(service_refresh_interval, service_refresh_interval_ms)) {
            return -1;
        }
        ptr->service_refresh_interval = service_refresh_interval_ms;
        std::string service_expire_time = local_cache["serviceExpireTime"].as<std::string>("24h");
        uint64_t service_expire_time_ms;
        if (!ParseTimeValue(service_expire_time, service_expire_time_ms)) {
            return -1;
        }
        ptr->service_expire_time = service_expire_time_ms;
    }
    // init circuitBreaker config
    if (consumer["circuitBreaker"].IsDefined() && !consumer["circuitBreaker"].IsNull()) {
        YAML::Node circuit_breaker = consumer["circuitBreaker"];
        ptr->circuit_breaker_enable = circuit_breaker["enable"].as<bool>(true);
        std::string circuit_breaker_check_period =
            circuit_breaker["checkPeriod"].as<std::string>("500ms");
        uint64_t circuit_breaker_check_period_ms;
        if (!ParseTimeValue(circuit_breaker_check_period, circuit_breaker_check_period_ms)) {
            return -1;
        }
        ptr->circuit_breaker_check_period = circuit_breaker_check_period_ms;
        if (circuit_breaker["chain"].IsDefined()) {
            YAML::Node circuit_breaker_chain = circuit_breaker["chain"];
            if (circuit_breaker_chain.size() > 0) {
                ptr->circuit_breaker_chain.clear();
                for (YAML::const_iterator iter = circuit_breaker_chain.begin();
                     iter != circuit_breaker_chain.end(); ++iter) {
                    ptr->circuit_breaker_chain.push_back(iter->as<std::string>());
                }
            }
        }
        if (circuit_breaker["plugin"].IsDefined()) {
            if (circuit_breaker["plugin"]["errCount"].IsDefined() &&
                !circuit_breaker["plugin"]["errCount"].IsNull()) {
                YAML::Node error_count = circuit_breaker["plugin"]["errCount"];
                ptr->error_count_request_threshold =
                    error_count["continuousErrorThreshold"].as<int>(10);
                std::string error_count_stat_time_window =
                    error_count["metricStatTimeWindow"].as<std::string>("1m");
                uint64_t error_count_stat_time_window_ms;
                if (!ParseTimeValue(error_count_stat_time_window,
                                    error_count_stat_time_window_ms)) {
                    return -1;
                }
                ptr->error_count_stat_time_window = error_count_stat_time_window_ms;
                std::string error_count_sleep_window =
                    error_count["sleepWindow"].as<std::string>("5s");
                uint64_t error_count_sleep_window_ms;
                if (!ParseTimeValue(error_count_sleep_window, error_count_sleep_window_ms)) {
                    return -1;
                }
                ptr->error_count_sleep_window = error_count_sleep_window_ms;
                ptr->error_count_max_request_halfopen =
                    error_count["requestCountAfterHalfOpen"].as<int>(3);
                ptr->error_count_least_success_halfopen =
                    error_count["successCountAfterHalfOpen"].as<int>(2);
            }
            if (circuit_breaker["plugin"]["errRate"].IsDefined() &&
                !circuit_breaker["plugin"]["errRate"].IsNull()) {
                YAML::Node error_rate = circuit_breaker["plugin"]["errRate"];
                ptr->error_rate_request_threshold =
                    error_rate["requestVolumeThreshold"].as<int>(10);
                ptr->error_rate_threshold = error_rate["errorRateThreshold"].as<double>(0.5);
                std::string error_rate_stat_time_window =
                    error_rate["metricStatTimeWindow"].as<std::string>("1m");
                uint64_t error_rate_stat_time_window_ms;
                if (!ParseTimeValue(error_rate_stat_time_window, error_rate_stat_time_window_ms)) {
                    return -1;
                }
                ptr->error_rate_stat_time_window = error_rate_stat_time_window_ms;
                ptr->error_rate_num_buckets = error_rate["metricNumBuckets"].as<int>(12);
                std::string error_rate_sleep_window =
                    error_rate["sleepWindow"].as<std::string>("5s");
                uint64_t error_rate_sleep_window_ms;
                if (!ParseTimeValue(error_rate_sleep_window, error_rate_sleep_window_ms)) {
                    return -1;
                }
                ptr->error_rate_sleep_window = error_rate_sleep_window_ms;
                ptr->error_rate_max_request_halfopen =
                    error_rate["requestCountAfterHalfOpen"].as<int>(3);
                ptr->error_rate_least_success_halfopen =
                    error_rate["successCountAfterHalfOpen"].as<int>(2);
            }
        }

        if (circuit_breaker["setCircuitBreaker"].IsDefined()) {
            ptr->setcluster_circuit_breaker_enable =
                circuit_breaker["setCircuitBreaker"]["enable"].as<bool>(false);
        }
    }

    // init healthCheck config
    if (consumer["healthCheck"].IsDefined() && !consumer["healthCheck"].IsNull()) {
        YAML::Node health_check = consumer["healthCheck"];
        ptr->health_check_enable = health_check["enable"].as<bool>(true);
        std::string health_check_period = health_check["checkPeriod"].as<std::string>("10s");
        uint64_t health_check_period_ms;
        if (!ParseTimeValue(health_check_period, health_check_period_ms)) {
            return -1;
        }
        ptr->health_check_period = health_check_period_ms;
        if (health_check["chain"].IsDefined()) {
            YAML::Node health_check_chain = health_check["chain"];
            if (health_check_chain.size() > 0) {
                ptr->health_check_chain.clear();
                for (YAML::const_iterator iter = health_check_chain.begin();
                     iter != health_check_chain.end(); ++iter) {
                    ptr->health_check_chain.push_back(iter->as<std::string>());
                }
            }
        }
        if (health_check["plugin"].IsDefined()) {
            if (health_check["plugin"]["tcp"].IsDefined() &&
                !health_check["plugin"]["tcp"].IsNull()) {
                YAML::Node tcp = health_check["plugin"]["tcp"];
                std::string tcp_timeout = tcp["timeout"].as<std::string>("100ms");
                uint64_t tcp_timeout_ms;
                if (!ParseTimeValue(tcp_timeout, tcp_timeout_ms)) {
                    return -1;
                }
                ptr->plugin_tcp_timeout = tcp_timeout_ms;
                ptr->plugin_tcp_retry = tcp["retry"].as<int>(0);
                if (tcp["send"].IsDefined() && !tcp["send"].IsNull()) {
                    ptr->plugin_tcp_send = tcp["send"].as<std::string>();
                }
                if (tcp["receive"].IsDefined() && !tcp["receive"].IsNull()) {
                    ptr->plugin_tcp_receive = tcp["receive"].as<std::string>();
                }
            }
            if (health_check["plugin"]["udp"].IsDefined() &&
                !health_check["plugin"]["udp"].IsNull()) {
                YAML::Node udp = health_check["plugin"]["udp"];
                std::string plugin_udp_timeout = udp["timeout"].as<std::string>("100ms");
                uint64_t plugin_udp_timeout_ms;
                if (!ParseTimeValue(plugin_udp_timeout, plugin_udp_timeout_ms)) {
                    return -1;
                }
                ptr->plugin_udp_timeout = plugin_udp_timeout_ms;
                ptr->plugin_udp_retry = udp["retry"].as<int>(0);
                if (udp["send"].IsDefined() && !udp["send"].IsNull()) {
                    ptr->plugin_udp_send = udp["send"].as<std::string>();
                }
                if (udp["receive"].IsDefined() && !udp["receive"].IsNull()) {
                    ptr->plugin_udp_receive = udp["receive"].as<std::string>();
                }
            }
            if (health_check["plugin"]["http"].IsDefined() &&
                !health_check["plugin"]["http"].IsNull()) {
                YAML::Node http = health_check["plugin"]["http"];
                std::string plugin_http_timeout = http["timeout"].as<std::string>("100ms");
                uint64_t plugin_http_timeout_ms;
                if (!ParseTimeValue(plugin_http_timeout, plugin_http_timeout_ms)) {
                    return -1;
                }
                ptr->plugin_http_timeout = plugin_http_timeout_ms;
                ptr->plugin_http_path = http["path"].as<std::string>("/ping");
            }
        }
    }
    // init loadBalancer config
    if (consumer["loadBalancer"].IsDefined() && !consumer["loadBalancer"].IsNull()) {
        ptr->load_balancer_type =
            consumer["loadBalancer"]["type"].as<std::string>("weightedRandom");
    }
    // init serviceRouter config
    if (consumer["serviceRouter"].IsDefined() && !consumer["serviceRouter"].IsNull()) {
        YAML::Node service_router = consumer["serviceRouter"];
        if (service_router["chain"].IsDefined()) {
            YAML::Node service_router_chain = service_router["chain"];
            if (service_router_chain.size() > 0) {
                ptr->service_router_chain.clear();
                for (YAML::const_iterator iter = service_router_chain.begin();
                     iter != service_router_chain.end(); ++iter) {
                    ptr->service_router_chain.push_back(iter->as<std::string>());
                }
            }
        }
        if (service_router["plugin"].IsDefined()) {
            if (service_router["plugin"]["nearbyBasedRouter"].IsDefined() &&
                !service_router["plugin"]["nearbyBasedRouter"].IsNull()) {
                YAML::Node nearby = service_router["plugin"]["nearbyBasedRouter"];
                ptr->nearby_match_level = nearby["matchLevel"].as<std::string>("zone");
                ptr->nearby_max_match_level = nearby["maxMatchLevel"].as<std::string>("none");
                ptr->nearby_unhealthy_degrade =
                    nearby["enableDegradeByUnhealthyPercent"].as<bool>(true);
                ptr->nearby_unhealthy_degrade_percent =
                    nearby["unhealthyPercentToDegrade"].as<int>(100);
                ptr->nearby_enable_recover_all = nearby["enableRecoverAll"].as<bool>(true);
                ptr->nearby_strict_nearby = nearby["strictNearby"].as<bool>(false);
            }
        }
    }
    return 0;
}

int init_ratelimiter_from_yaml(struct polaris_config *ptr, const YAML::Node &node) {
    // init rateLimiter config
    if (!node["rateLimiter"].IsDefined()) {
        return 0;
    }
    YAML::Node rate_limiter = node["rateLimiter"];
    ptr->rate_limit_mode = rate_limiter["mode"].as<std::string>("local");
    if (rate_limiter["rateLimitCluster"].IsDefined() &&
        !rate_limiter["rateLimitCluster"].IsNull()) {
        YAML::Node cluster = rate_limiter["rateLimitCluster"];
        ptr->rate_limit_cluster_namespace = cluster["namespace"].as<std::string>("Polaris");
        ptr->rate_limit_cluster_name = cluster["service"].as<std::string>("polaris.metric");
    }
    return 0;
}

void PolarisInstance::instance_init() {
    this->inst->enable_healthcheck = false;
    this->inst->healthy = true;
    this->inst->isolate = false;
    this->inst->weight = 100;
    this->inst->healthcheck_type = 1;
    this->inst->healthcheck_ttl = 5;
}

void PolarisConfig::polaris_config_init_global() {
    this->ptr->discover_namespace = "Polaris";
    this->ptr->discover_name = "polaris.discover";
    this->ptr->discover_refresh_interval = 6000000;  // 10 * 60 * 1000 milliseconds
    this->ptr->healthcheck_namespace = "Polaris";
    this->ptr->healthcheck_name = "polaris.healthcheck";
    this->ptr->healthcheck_refresh_interval = 6000000;
    this->ptr->monitor_namespace = "Polaris";
    this->ptr->monitor_name = "polaris.monitor";
    this->ptr->monitor_refresh_interval = 6000000;

    this->ptr->metric_namespace = "Polaris";
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

void PolarisConfig::polaris_config_init_consumer() {
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
    this->ptr->nearby_strict_nearby = false;
}

void PolarisConfig::polaris_config_init_ratelimiter() {
    this->ptr->rate_limit_mode = "local";
    this->ptr->rate_limit_cluster_namespace = "Polaris";
    this->ptr->rate_limit_cluster_name = "polaris.metric";
}

int PolarisConfig::init_from_yaml(const std::string &yaml_file) {
    try {
        YAML::Node root;
        root = YAML::LoadFile(yaml_file);
        int ret = 0;
        ret = init_global_from_yaml(this->ptr, root);
        if (ret != 0) return -1;
        ret = init_consumer_from_yaml(this->ptr, root);
        if (ret != 0) return -1;
        ret = init_ratelimiter_from_yaml(this->ptr, root);
        if (ret != 0) return -1;
    } catch (const YAML::Exception &e) {
        return -1;
    }
    return 0;
}

};  // namespace polaris
