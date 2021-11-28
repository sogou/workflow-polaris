#include "src/PolarisTask.h"
#include "src/PolarisClient.h"
#include "src/json.hpp"
//#include <nlohmann/json.hpp>

using nlohmann::json;

namespace polaris {

#define REDIRECT_MAX 5

void to_json(json &j, const struct discover_request &request);
void to_json(json &j, const struct register_request &request);
void to_json(json &j, const struct deregister_request &request);
void to_json(json &j, const struct ratelimit_request &request);
void to_json(json &j, const struct circuitbreaker_request &request);
void from_json(const json &j, struct discover_result &response);
void from_json(const json &j, struct route_result &response);
void from_json(const json &j, struct ratelimit_result &response);
void from_json(const json &j, struct circuitbreaker_result &response);

void PolarisTask::dispatch() {
    if (this->finish) {
        this->subtask_done();
        return;
    }
    SubTask *task;
    this->cluster.get_mutex()->lock();
    // todo: set cluster ttl for update
    if (!(*this->cluster.get_status() & POLARIS_DISCOVER_CLUSTZER_INITED)) {
        if (this->protocol == P_HTTP) {
            task = create_cluster_http_task();
        }
    } else {
        if (this->protocol == P_UNKNOWN) {
            task = WFTaskFactory::create_empty_task();
        } else {
            switch (this->apitype) {
                case API_DISCOVER:
                    if (this->protocol == P_HTTP) {
                        task = create_instances_http_task();
                        break;
                    }
                case API_REGISTER:
                    if (this->protocol == P_HTTP) {
                        task = create_register_http_task();
                        break;
                    }
                case API_DEREGISTER:
                    if (this->protocol == P_HTTP) {
                        task = create_deregister_http_task();
                        break;
                    }
                case API_RATELIMIT:
                    if (this->protocol == P_HTTP) {
                        task = create_ratelimit_http_task();
                        break;
                    }
                default:
                    task = WFTaskFactory::create_empty_task();
                    break;
            }
        }
    }
    this->cluster.get_mutex()->unlock();
    series_of(this)->push_front(task);
    this->subtask_done();
}

SubTask *PolarisTask::done() {
    SeriesWork *series = series_of(this);
    if (this->finish) {
        if (this->callback) {
            this->callback(this);
        }

        delete this;
    }
    return series->pop();
}

WFHttpTask *PolarisTask::create_cluster_http_task() {
    std::string url = this->url + "/v1/Discover";
    auto *task =
        WFTaskFactory::create_http_task(url, REDIRECT_MAX, this->retry_max, cluster_http_callback);
    task->user_data = this;
    protocol::HttpRequest *req = task->get_req();
    req->set_method(HttpMethodPost);
    req->add_header_pair("Content-Type", "application/json");
    struct discover_request request {
        .type = INSTANCE, .service_name = this->config.get_polaris_config()->discover_name,
        .service_namespace = this->config.get_polaris_config()->discover_namespace, .revision = "0",
    };
    std::string output = create_discover_request(request);
    req->append_output_body(output.c_str(), output.length());
    series_of(this)->push_front(this);
    return task;
}

WFHttpTask *PolarisTask::create_instances_http_task() {
    int pos = rand() % this->cluster.get_discover_clusters()->size();
    std::string url = this->cluster.get_discover_clusters()->at(pos) + "/v1/Discover";

    auto *task = WFTaskFactory::create_http_task(url, REDIRECT_MAX, this->retry_max,
                                                 instances_http_callback);
    protocol::HttpRequest *req = task->get_req();
    task->user_data = this;
    req->set_method(HttpMethodPost);
    req->add_header_pair("Content-Type", "application/json");
    std::string servicekey = this->service_name + "." + this->service_namespace;
    std::string revision = this->cluster.get_revision_map()->count(servicekey)
                               ? (*this->cluster.get_revision_map())[servicekey]
                               : "0";
    struct discover_request request {
        .type = INSTANCE, .service_name = this->service_name,
        .service_namespace = this->service_namespace, .revision = revision,
    };
    std::string output = create_discover_request(request);
    req->append_output_body(output.c_str(), output.length());
    series_of(this)->push_front(this);
    return task;
}

WFHttpTask *PolarisTask::create_route_http_task() {
    int pos = rand() % this->cluster.get_discover_clusters()->size();
    std::string url = this->cluster.get_discover_clusters()->at(pos) + "/v1/Discover";
    auto *task =
        WFTaskFactory::create_http_task(url, REDIRECT_MAX, this->retry_max, route_http_callback);
    task->user_data = this;
    protocol::HttpRequest *req = task->get_req();
    req->set_method(HttpMethodPost);
    req->add_header_pair("Content-Type", "application/json");
    // todo: use route revision instead of 0
    struct discover_request request {
        .type = ROUTING, .service_name = this->service_name,
        .service_namespace = this->service_namespace, .revision = "0",
    };
    std::string output = create_discover_request(request);
    req->append_output_body(output.c_str(), output.length());
    return task;
}

WFHttpTask *PolarisTask::create_register_http_task() {
    int pos = rand() % this->cluster.get_discover_clusters()->size();
    std::string url = this->cluster.get_discover_clusters()->at(pos) + "/v1/RegisterInstance";
    auto *task =
        WFTaskFactory::create_http_task(url, REDIRECT_MAX, this->retry_max, register_http_callback);
    protocol::HttpRequest *req = task->get_req();
    task->user_data = this;
    req->set_method(HttpMethodPost);
    req->add_header_pair("Content-Type", "application/json");
    struct register_request request {
        .service = this->service_name, .service_namespace = this->service_namespace
    };
    request.inst = *this->polaris_instance.get_instance();
    if (!this->service_token.empty()) {
        request.service_token = this->service_token;
    }
    std::string output = create_register_request(request);
    req->append_output_body(output.c_str(), output.length());
    series_of(this)->push_front(this);
    return task;
}

WFHttpTask *PolarisTask::create_deregister_http_task() {
    int pos = rand() % this->cluster.get_discover_clusters()->size();
    std::string url = this->cluster.get_discover_clusters()->at(pos) + "/v1/DeregisterInstance";
    auto *task =
        WFTaskFactory::create_http_task(url, REDIRECT_MAX, this->retry_max, register_http_callback);
    protocol::HttpRequest *req = task->get_req();
    task->user_data = this;
    req->set_method(HttpMethodPost);
    req->add_header_pair("Content-Type", "application/json");
    struct deregister_request request;
    if (!this->polaris_instance.get_instance()->id.empty()) {
        request.id = this->polaris_instance.get_instance()->id;
    } else {
        request.service = this->service_name;
        request.service_namespace = this->service_namespace;
        request.host = this->polaris_instance.get_instance()->host;
        request.port = this->polaris_instance.get_instance()->port;
    }
    if (!this->service_token.empty()) {
        request.service_token = this->service_token;
    }
    std::string output = create_deregister_request(request);
    req->append_output_body(output.c_str(), output.length());
    series_of(this)->push_front(this);
    return task;
}

WFHttpTask *PolarisTask::create_ratelimit_http_task() {
    int pos = rand() % this->cluster.get_discover_clusters()->size();
    std::string url = this->cluster.get_discover_clusters()->at(pos) + "/v1/Discover";
    auto *task = WFTaskFactory::create_http_task(url, REDIRECT_MAX, this->retry_max,
                                                 ratelimit_http_callback);
    protocol::HttpRequest *req = task->get_req();
    task->user_data = this;
    req->set_method(HttpMethodPost);
    req->add_header_pair("Content-Type", "application/json");
    struct ratelimit_request request {
        .type = RATE_LIMIT, .service_name = this->service_name,
        .service_namespace = this->service_namespace, .revision = 0
    };
    std::string output = create_ratelimit_request(request);
    req->append_output_body(output.c_str(), output.length());
    series_of(this)->push_front(this);
    return task;
}

WFHttpTask *PolarisTask::create_circuitbreaker_http_task() {
    int pos = rand() % this->cluster.get_discover_clusters()->size();
    std::string url = this->cluster.get_discover_clusters()->at(pos) + "/v1/Discover";
    auto *task = WFTaskFactory::create_http_task(url, REDIRECT_MAX, this->retry_max,
                                                 circuitbreaker_http_callback);
    protocol::HttpRequest *req = task->get_req();
    task->user_data = this;
    req->set_method(HttpMethodPost);
    req->add_header_pair("Content-Type", "application/json");
    struct circuitbreaker_request request {
        .type = CIRCUIT_BREAKER, .service_name = this->service_name,
        .service_namespace = this->service_namespace, .revision = 0
    };
    std::string output = create_circuitbreaker_request(request);
    req->append_output_body(output.c_str(), output.length());
    series_of(this)->push_front(this);
    return task;
}

void PolarisTask::cluster_http_callback(WFHttpTask *task) {
    PolarisTask *t = (PolarisTask *)task->user_data;
    t->cluster.get_mutex()->lock();
    if (task->get_state() == WFT_STATE_SUCCESS) {
        protocol::HttpResponse *resp = task->get_resp();
        std::string revision;
        std::string body = protocol::HttpUtil::decode_chunked_body(resp);
        if (!t->parse_cluster_response(body, revision)) {
            t->state = WFT_STATE_TASK_ERROR;
            t->finish = true;
        } else {
            *t->cluster.get_status() |= POLARIS_DISCOVER_CLUSTZER_INITED;
            std::string servicekey = t->config.get_polaris_config()->discover_namespace + "." +
                                     t->config.get_polaris_config()->discover_name;
            (*t->cluster.get_revision_map())[servicekey] = revision;
        }
    } else {
        t->state = task->get_state();
        t->error = task->get_error();
        t->finish = true;
    }
    t->cluster.get_mutex()->unlock();
}

void PolarisTask::instances_http_callback(WFHttpTask *task) {
    PolarisTask *t = (PolarisTask *)task->user_data;
    t->cluster.get_mutex()->lock();
    if (task->get_state() == WFT_STATE_SUCCESS) {
        protocol::HttpResponse *resp = task->get_resp();
        std::string revision;
        std::string body = protocol::HttpUtil::decode_chunked_body(resp);
        if (!t->parse_instances_response(body, revision)) {
            t->state = WFT_STATE_TASK_ERROR;
            t->finish = true;
        } else {
            std::string servicekey = t->service_namespace + "." + t->service_name;
            (*t->cluster.get_revision_map())[servicekey] = revision;
            auto *task = t->create_route_http_task();
            series_of(t)->push_front(task);
        }
    } else {
        t->state = task->get_state();
        t->error = task->get_error();
        t->finish = true;
    }
    t->cluster.get_mutex()->unlock();
}

void PolarisTask::route_http_callback(WFHttpTask *task) {
    PolarisTask *t = (PolarisTask *)task->user_data;
    t->cluster.get_mutex()->lock();
    if (task->get_state() == WFT_STATE_SUCCESS) {
        protocol::HttpResponse *resp = task->get_resp();
        std::string revision;
        std::string body = protocol::HttpUtil::decode_chunked_body(resp);
        if (!t->parse_route_response(body, revision)) {
            t->state = WFT_STATE_TASK_ERROR;
        } else {
            t->state = task->get_state();
        }
    } else {
        t->state = task->get_state();
        t->error = task->get_error();
    }
    t->finish = true;
    t->cluster.get_mutex()->unlock();
}

void PolarisTask::register_http_callback(WFHttpTask *task) {
    PolarisTask *t = (PolarisTask *)task->user_data;
    if (task->get_state() == WFT_STATE_SUCCESS) {
        protocol::HttpResponse *resp = task->get_resp();
        std::string body = protocol::HttpUtil::decode_chunked_body(resp);
        if (!t->parse_register_response(body)) {
            t->state = WFT_STATE_TASK_ERROR;
        } else {
            t->state = task->get_state();
        }
    } else {
        t->state = task->get_state();
        t->error = task->get_error();
    }
    t->finish = true;
}

void PolarisTask::ratelimit_http_callback(WFHttpTask *task) {
    PolarisTask *t = (PolarisTask *)task->user_data;
    if (task->get_state() == WFT_STATE_SUCCESS) {
        std::string revision;
        protocol::HttpResponse *resp = task->get_resp();
        std::string body = protocol::HttpUtil::decode_chunked_body(resp);
        if (!t->parse_ratelimit_response(body, revision)) {
            t->state = WFT_STATE_TASK_ERROR;
        } else {
            t->state = task->get_state();
        }
    } else {
        t->state = task->get_state();
        t->error = task->get_error();
    }
    t->finish = true;
}

void PolarisTask::circuitbreaker_http_callback(WFHttpTask *task) {
    PolarisTask *t = (PolarisTask *)task->user_data;
    if (task->get_state() == WFT_STATE_SUCCESS) {
        std::string revision;
        protocol::HttpResponse *resp = task->get_resp();
        std::string body = protocol::HttpUtil::decode_chunked_body(resp);
        if (!t->parse_circuitbreaker_response(body, revision)) {
            t->state = WFT_STATE_TASK_ERROR;
        } else {
            t->state = task->get_state();
        }
    } else {
        t->state = task->get_state();
        t->error = task->get_error();
    }
    t->finish = true;
}

std::string PolarisTask::create_discover_request(const struct discover_request &request) {
    const json j = request;
    return j.dump();
}

std::string PolarisTask::create_register_request(const struct register_request &request) {
    const json j = request;
    return j.dump();
}

std::string PolarisTask::create_deregister_request(const struct deregister_request &request) {
    const json j = request;
    return j.dump();
}

std::string PolarisTask::create_ratelimit_request(const struct ratelimit_request &request) {
    const json j = request;
    return j.dump();
}

std::string PolarisTask::create_circuitbreaker_request(
    const struct circuitbreaker_request &request) {
    const json j = request;
    return j.dump();
}

bool PolarisTask::parse_cluster_response(const std::string &body, std::string &revision) {
    json j = json::parse(body, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    struct discover_result response = j;
    if (response.instances.empty()) {
        return false;
    } else {
        if (response.code != 200001) {
            this->cluster.get_discover_clusters()->clear();
            auto iter = response.instances.begin();
            for (; iter != response.instances.end(); iter++) {
                if (strcmp((iter->protocol).c_str(), "http") == 0) {
                    std::string url = "http://" + iter->host + ":" + std::to_string(iter->port);
                    this->cluster.get_discover_clusters()->emplace_back(url);
                }
            }
        }
    }
    revision = response.service_revision;
    return true;
}

bool PolarisTask::parse_instances_response(const std::string &body, std::string &revision) {
    json j = json::parse(body, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    int code = j.at("code").get<int>();
    if (code != 200000 && code != 200001) {
        return false;
    }
    revision = j.at("service").at("revision").get<std::string>();
    this->discover_res = body;
    return true;
}

bool PolarisTask::parse_route_response(const std::string &body, std::string &revision) {
    json j = json::parse(body, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    int code = j.at("code").get<int>();
    if (code != 200000 && code != 200001) {
        return false;
    }
    this->route_res = body;
    revision = j.at("routing").at("revision").get<std::string>();
    return true;
}

bool PolarisTask::parse_register_response(const std::string &body) {
    json j = json::parse(body, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    int code = j.at("code").get<int>();
    if (code != 200000 && code != 200001) {
        if (code == 400201)
            return true;  // todo: existed resource err, should update if existed later
        return false;
    }
    return true;
}

bool PolarisTask::parse_ratelimit_response(const std::string &body, std::string &revision) {
    json j = json::parse(body, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    int code = j.at("code").get<int>();
    if (code != 200000 && code != 200001) {
        return false;
    }
    this->ratelimit_res = body;
    revision = j.at("routing").at("revision").get<std::string>();
    return true;
}

bool PolarisTask::parse_circuitbreaker_response(const std::string &body, std::string &revision) {
    json j = json::parse(body, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    int code = j.at("code").get<int>();
    if (code != 200000 && code != 200001) {
        return false;
    }
    this->circuitbreaker_res = body;
    revision = j.at("routing").at("revision").get<std::string>();
    return true;
}

bool PolarisTask::get_discover_result(struct discover_result *result) const {
    if (this->discover_res.empty()) return false;
    json j = json::parse(this->discover_res, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    *result = j;
    return true;
}

bool PolarisTask::get_route_result(struct route_result *result) const {
    if (this->route_res.empty()) return false;
    json j = json::parse(this->route_res, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    *result = j;
    return true;
}

bool PolarisTask::get_ratelimit_result(struct ratelimit_result *result) const {
    if (this->ratelimit_res.empty()) return false;
    json j = json::parse(this->ratelimit_res, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    *result = j;
    return true;
}

bool PolarisTask::get_circuitbreaker_result(struct circuitbreaker_result *result) const {
    if (this->circuitbreaker_res.empty()) return false;
    json j = json::parse(this->circuitbreaker_res, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    *result = j;
    return true;
}

};  // namespace polaris
