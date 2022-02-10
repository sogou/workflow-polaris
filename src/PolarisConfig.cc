#include "PolarisConfig.h"
#include "json.hpp"

using nlohmann::json;

namespace polaris {

static const int kDefaultInstancePort = 80;
static const int kDefaultInstancePriority = 0;
static const int kDefaultInstanceWeight = 100;

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
    if (j.find("enableHealthCheck") != j.end()) {
        j.at("healthCheck").at("type").get_to(response.healthcheck_type);
        j.at("healthCheck").at("heartbeat").at("ttl").get_to(response.healthcheck_ttl);
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

};  // namespace polaris
