#include "src/PolarisConfig.h"

namespace polaris {

void to_json(json &j, const struct discover_request &request) {
    j = json{{"type", request.type},
             {"service",
              {{"name", request.service_name},
               {"namespace", request.service_namespace},
               {"revision", request.revision}}}};
}

void from_json(const json &j, struct instance &response) {
    j.at("id").get_to(response.id);
    j.at("service").get_to(response.service);
    j.at("namespace").get_to(response.service_namespace);
    j.at("vpc_id").get_to(response.vpc_id);
    j.at("host").get_to(response.host);
    j.at("port").get_to(response.port);
    j.at("protocol").get_to(response.protocol);
    j.at("version").get_to(response.version);
    j.at("priority").get_to(response.priority);
    j.at("weight").get_to(response.weight);
    j.at("enableHealthCheck").get_to(response.enable_healthcheck);
    j.at("healthy").get_to(response.healthy);
    j.at("isolate").get_to(response.isolate);
    j.at("metadata").at("protocol").get_to(response.metadata_protocol);
    j.at("metadata").at("version").get_to(response.metadata_version);
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

void from_json(const json &j, struct label &response) {
    if (j.find("type") != j.end()) {
        j.at("type").get_to(response.type);
    }
    j.at("value").get_to(response.value);
}

void from_json(const json &j, struct source_bound &response) {
    j.at("service").get_to(response.service);
    j.at("namespace").get_to(response.service_namespace);
    response.metadata.clear();
    j.at("metadata").get_to<std::map<std::string, struct label>>(response.metadata);
}

void from_json(const json &j, struct destination_bound &response) {
    j.at("service").get_to(response.service);
    j.at("namespace").get_to(response.service_namespace);
    response.metadata.clear();
    j.at("metadata").get_to<std::map<std::string, struct label>>(response.metadata);
    j.at("priority").get_to(response.priority);
    j.at("weight").get_to(response.weight);
}

void from_json(const json &j, struct bound &response) {
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
                    .get_to<std::vector<struct bound>>(response.routing_inbounds);
                response.routing_outbounds.clear();
                j.at("routing")
                    .at("inbounds")
                    .get_to<std::vector<struct bound>>(response.routing_outbounds);
                j.at("routing").at("ctime").get_to(response.routing_ctime);
                j.at("routing").at("mtime").get_to(response.routing_mtime);
                j.at("routing").at("revision").get_to(response.routing_revision);
            }
            break;
        default:
            break;
    }
}

};  // namespace polaris
