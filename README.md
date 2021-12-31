# workflow-polaris
## 介绍

本项目是基于腾讯开源的服务发现&服务治理平台[北极星](https://polarismesh.cn/#/)上构建，目标是将workflow的服务治理能力和北极星治理能力相结合，提供更便捷、更丰富的服务场景。另外其他同型的服务治理系统也可以通过该项目实现与北极星平台的对接。

功能：

- 基础功能：服务发现、服务注册
- 流量控制：负载均衡，路由管理

## Demo

### Discover
```cpp
#include "PolarisClient.h"
#include "PolarisTask.h"
#include "workflow/WFFacilities.h"
#include <signal.h>

#define RETRY_MAX 5

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

	//get discover results and do something
	struct discover_result discover;
	if (!task->get_discover_result(&discover)) {
		fprintf(stderr, "get discover_result error: %d\n", error);
		client.deinit();
		wait_group.done();
		return;
	}

	struct route_result route;
	if (!task->get_route_result(&route)) {
		fprintf(stderr, "get route_result error: %d\n", error);
		client.deinit();
		wait_group.done();
		return;
	}

	fprintf(stderr, "\nSuccess. Press Ctrl-C to exit.\n");
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
		client.create_discover_task("your.namespace", "your.service.name",
									RETRY_MAX, polaris_callback);

	PolarisConfig config;
	task->set_config(std::move(config));
	task->start();

	wait_group.wait();
	return 0;
}
```

### Register&&Deregister

```cpp
#include "PolarisClient.h"
#include "PolarisTask.h"
#include "workflow/WFFacilities.h"
#include <signal.h>

#define RETRY_MAX 5

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
	fprintf(stderr, "Task ok\n");
}

void sig_handler(int signo) { wait_group.done(); }

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "USAGE: %s [r/d]\n", argv[0]);
		exit(1);
	}

	PolarisTask *task;
	signal(SIGINT, sig_handler);
	std::string url = "http://your.polaris.cluster:8090";
	int ret = client.init(url);
	if (ret != 0) {
		exit(1);
	}

	PolarisConfig config;
	if (argv[1][0] == 'r') {
		task = client.create_register_task("your.namespace",
										   "your.service_name",
										   RETRY_MAX,
										   polaris_callback);
		task->set_config(std::move(config));
		PolarisInstance instance;
		instance.set_host("your.instance.ip");
		instance.set_port(8080);
		std::map<std::string, std::string> meta = {{"key1", "value1"}};
		instance.set_metadata(meta);
		task->set_polaris_instance(std::move(instance));

	} else if (argv[1][0] == 'd') {
		task = client.create_deregister_task("your.namespace",
											 "your.service_name",
											 RETRY_MAX,
											 polaris_callback);
		task->set_config(std::move(config));
		PolarisInstance instance;
		instance.set_host("your.instance.ip");
		instance.set_port(8080);
		task->set_polaris_instance(std::move(instance));

	} else {
		fprintf(stderr, "USAGE: %s [r/d]\n", argv[0]);
		exit(1);
	}

	task->start();
	wait_group.wait();
	return 0;
}
```
