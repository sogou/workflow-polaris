# workflow-polaris


## Demo
```cpp
#include "PolarisClient.h"
#include "PolarisTask.h"
#include "workflow/WFFacilities.h"
#include <signal.h>

using namespace polaris;
static WFFacilities::WaitGroup wait_group(1);
PolarisClient client;

void polaris_callback(PolarisTask *task) {
    int state = task->get_state();
    int error = task->get_error();
    if (state != WFT_STATE_SUCCESS) {
        fprintf(stderr, "Task error: %d\n", error);
        client.deinit();
        wait_group.done();
        return;
    }
	//get discover results(instance, route) and do something
    const discover_response_t *resp = task->get_instances();
    const route_response_t *route = task->get_route();
}

void sig_handler(int signo) { wait_group.done(); }

int main(int argc, char *argv[]) {
    PolarisTask *task;

    signal(SIGINT, sig_handler);
    std::string url = "http://your.polaris.cluster:8090";
    int ret = client.init(url);
    if (ret != 0) {
        client.deinit();
        exit(1);
    }
    task =
        client.create_discover_task("your.namespace", "your.service.name", 5, polaris_callback);

    PolarisConfig config;
    task->set_config(std::move(config));
    task->start();

    wait_group.wait();
    return 0;
}
```
