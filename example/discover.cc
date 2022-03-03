#include <string>
#include "src/PolarisClient.h"
#include "src/PolarisTask.h"
#include "src/PolarisPolicies.h"
#include "workflow/WFFacilities.h"
#include "workflow/WFHttpServer.h"

#include <signal.h>

#define RETRY_MAX 5
#define REDIRECT_MAX 3
#define INSTANCES_NUM 4

using namespace polaris;

static WFFacilities::WaitGroup polaris_wait_group(1);
static WFFacilities::WaitGroup query_wait_group(1);

static const std::string service_namespace = "default";
static const std::string service_name = "workflow.polaris.service.b";

PolarisClient client;
PolarisConfig config;
WFHttpServer *instances[INSTANCES_NUM];
bool discover_success = false;

void polaris_callback(PolarisTask *task) {
    int state = task->get_state();
    int error = task->get_error();

    if (state != WFT_STATE_SUCCESS) {
        fprintf(stderr, "Task state: %d error: %d\n", state, error);
        polaris_wait_group.done();
        return;
    }

    struct discover_result discover;
    if (!task->get_discover_result(&discover)) {
        fprintf(stderr, "Get discover_result error: %d\n", error);
        polaris_wait_group.done();
        return;
    }

    struct route_result route;
    if (!task->get_route_result(&route)) {
        fprintf(stderr, "Get route_result error: %d\n", error);
        polaris_wait_group.done();
        return;
    }

    fprintf(stderr, "Discover task success.\n");

    std::string policy_name = discover.service_namespace + "." + discover.service_name;

    WFNameService *ns = WFGlobal::get_name_service();
    PolarisPolicyConfig conf(policy_name);
    PolarisPolicy *pp = new PolarisPolicy(&conf);

    if (ns->add_policy(policy_name.c_str(), pp) < 0) {
        fprintf(stderr, "Policy %s existed.\n", policy_name.c_str());
        delete pp;
        pp = static_cast<PolarisPolicy *>(ns->get_policy(policy_name.c_str()));
    } else {
        fprintf(stderr, "Successfully add PolarisPolicy: %s\n", policy_name.c_str());
    }

    pp->update_instances(discover.instances);
    pp->update_inbounds(route.routing_inbounds);
    pp->update_outbounds(route.routing_outbounds);

    discover_success = true;
    polaris_wait_group.done();
}

bool init(const std::string &polaris_url) {
    int ret = client.init(polaris_url);
    if (ret != 0) return false;

    PolarisTask *task = client.create_discover_task(service_namespace.c_str(), service_name.c_str(),
                                                    RETRY_MAX, polaris_callback);
    task->set_config(config);
    task->start();

    polaris_wait_group.wait();

    if (discover_success) {
        for (size_t i = 0; i < INSTANCES_NUM; i++) {
            std::string port = "800";
            port += std::to_string(i);

            instances[i] = new WFHttpServer([port](WFHttpTask *task) {
                task->get_resp()->append_output_body("Response from instance 127.0.0.0.1:" + port);
            });

            if (instances[i]->start(atoi(port.c_str())) != 0) {
                fprintf(stderr, "Start instance 127.0.0.0.1:%s failed.\n", port.c_str());
                delete instances[i];
                instances[i] = NULL;
            }
        }
    } else {
        client.deinit();
    }

    return discover_success;
}

void deinit(const std::string &policy_name) {
    for (size_t i = 0; i < INSTANCES_NUM; i++) {
        if (instances[i]) {
            instances[i]->stop();
            delete instances[i];
        }
    }

    WFNSPolicy *pp = WFGlobal::get_name_service()->del_policy(policy_name.c_str());
    delete pp;

    client.deinit();
}

void sig_handler(int signo) {
    polaris_wait_group.done();
    query_wait_group.done();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <Polaris Cluster URL>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, sig_handler);

    std::string policy_name = service_namespace + "." + service_name;

    std::string polaris_url = argv[1];
    if (strncasecmp(argv[1], "http://", 7) != 0 && strncasecmp(argv[1], "https://", 8) != 0) {
        polaris_url = "http://" + polaris_url;
    }

    if (init(polaris_url) == false) return 0;

    std::string url =
        "http://" + policy_name + ":8080#k1_env=v1_base&k2_number=v2_prime&a_namespace.a";
    fprintf(stderr, "URL : %s\n", url.c_str());

    WFHttpTask *task =
        WFTaskFactory::create_http_task(url, REDIRECT_MAX, RETRY_MAX, [](WFHttpTask *task) {
            int state = task->get_state();
            int error = task->get_error();
            fprintf(stderr, "Query task callback. state = %d error = %d\n", state, error);

            if (state == WFT_STATE_SUCCESS) {
                const void *body;
                size_t body_len;
                task->get_resp()->get_parsed_body(&body, &body_len);
                fwrite(body, 1, body_len, stdout);
                fflush(stdout);
                fprintf(stderr, "\nSuccess. Press Ctrl-C to exit.\n");
            }
        });

    task->start();
    query_wait_group.wait();

    deinit(policy_name);

    return 0;
}

