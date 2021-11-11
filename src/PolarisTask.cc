#include "src/PolarisTask.h"
#include "src/PolarisClient.h"

namespace polaris {

#define REDIRECT_MAX 5

void PolarisTask::dispatch() {
    if (this->finish) {
        this->subtask_done();
        return;
    }
    this->cluster.get_mutex()->lock();
    // todo: set cluster ttl for update
    if (!(*this->cluster.get_status() & POLARIS_DISCOVER_CLUSTZER_INITED)) {
        if (this->protocol == P_HTTP) {
            WFHttpTask *task = create_cluster_http_task();
            series_of(this)->push_front(task);
        }
    } else {
        switch (this->apitype) {
            case DISCOVER:
                if (this->protocol == P_HTTP) {
                    WFHttpTask *task = create_instances_http_task();
                    series_of(this)->push_front(task);
                    break;
                }
            default:
				// empty or error task
                break;
        }
    }
    this->cluster.get_mutex()->unlock();
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
    auto *task = WFTaskFactory::create_http_task(url, REDIRECT_MAX, this->retry_max,
                                                 cluster_http_callback);
    task->user_data = this;
    protocol::HttpRequest *req = task->get_req();
    req->set_method(HttpMethodPost);
    req->add_header_pair("Content-Type", "application/json");
    struct discover_request request {
        .type = INSTANCE, .service_name = this->config.get_discover_name(),
        .service_namespace = this->config.get_discover_namespace(), .revision = "0",
    };
    std::string output = create_discover_request(request);
    req->append_output_body(output.c_str(), output.length());
    series_of(this)->push_front(this);
    return task;
}

WFHttpTask *PolarisTask::create_instances_http_task() {
    // todo: use upstream instead of random
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

    auto *task = WFTaskFactory::create_http_task(url, REDIRECT_MAX, this->retry_max,
                                                 route_http_callback);
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

void PolarisTask::cluster_http_callback(WFHttpTask *task) {
    PolarisTask *t = (PolarisTask *)task->user_data;
    t->cluster.get_mutex()->lock();
    if (task->get_state() == WFT_STATE_SUCCESS) {
        protocol::HttpResponse *resp = task->get_resp();
        // todo: parse cluster response, and init cluster
        const void *body;
        size_t body_len;
        std::string revision;
        resp->get_parsed_body(&body, &body_len);
        json j = json::parse((char *)body, ((char *)body) + body_len, nullptr, false);
        if (j.is_discarded() || !t->parse_cluster_response(j, revision)) {
            t->state = WFT_STATE_TASK_ERROR;
            t->finish = true;
        } else {
            *t->cluster.get_status() |= POLARIS_DISCOVER_CLUSTZER_INITED;
            std::string servicekey =
                t->config.get_discover_namespace() + "." + t->config.get_discover_name();
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
        const void *body;
        size_t body_len;
        std::string revision;
        resp->get_parsed_body(&body, &body_len);
        json j = json::parse((char *)body, (char *)body + body_len, nullptr, false);
        if (j.is_discarded() || !t->parse_instances_response(j, revision)) {
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
        const void *body;
        size_t body_len;
        std::string revision;
        resp->get_parsed_body(&body, &body_len);
        json j = json::parse((char *)body, (char *)body + body_len, nullptr, false);
        if (j.is_discarded() || !t->parse_route_response(j, revision)) {
            t->state = WFT_STATE_TASK_ERROR;
        } else {
            // std::string servicekey = t->service_namespace + "." + t->service_name;
            //*t->cluster.get_revision_map()[servicekey] = revision;
            t->state = task->get_state();
        }
    } else {
        t->state = task->get_state();
        t->error = task->get_error();
    }
    t->finish = true;
    t->cluster.get_mutex()->unlock();
}

std::string PolarisTask::create_discover_request(const discover_request_t &request) {
    const json j = request;
    return j.dump();
}

bool PolarisTask::parse_cluster_response(const json &j, std::string &revision) {
    if (!check_json(j)) {
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
                // todo add trpc url
            }
        }
    }
    revision = response.service_revision;
    return true;
}

bool PolarisTask::parse_instances_response(const json &j, std::string &revision) {
    if (!check_json(j)) {
        return false;
    }
    this->instances = j;
    revision = this->instances.service_revision;
    return true;
}

bool PolarisTask::parse_route_response(const json &j, std::string &revision) {
    if (!check_json(j)) {
        return false;
    }
    this->route = j;
    revision = this->route.routing_revision;
    return true;
}

};  // namespace polaris
