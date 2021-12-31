#include "src/PolarisClient.h"
#include "src/PolarisTask.h"
#include "src/PolarisPolicies.h"
#include "workflow/WFFacilities.h"

#include <signal.h>

#define RETRY_MAX 5

using namespace polaris;
static WFFacilities::WaitGroup wait_group(1);
PolarisClient client;
PolarisPolicy *pp = NULL;

void query() {
}

void polaris_callback(PolarisTask *task) {
	int state = task->get_state();
	int error = task->get_error();

	if (state != WFT_STATE_SUCCESS) {
		fprintf(stderr, "Task state: %d error: %d\n", state, error);
		client.deinit();
		wait_group.done();
		return;
	}

	// 1. get result
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

	fprintf(stderr, "Discover task success.\n");

	// 2. update policy
	const char *service_name = discover.service_name.c_str();
	WFNameService *ns = WFGlobal::get_name_service();
	PolarisPolicyConfig conf(service_name);
	pp = new PolarisPolicy(&conf);

	if (ns->add_policy(service_name, pp) < 0) {
		fprintf(stderr, "Policy %s existed.\n", service_name);
		delete pp;
		pp = static_cast<PolarisPolicy *>(ns->get_policy(service_name));
	}
	else
		fprintf(stderr, "Successfully add PolarisPolicy: %s\n", service_name);

	pp->update_instances(discover.instances);
	pp->update_inbounds(route.routing_inbounds);
	pp->update_outbounds(route.routing_outbounds);

	// 3. query
	EndpointAddress *addr = NULL;
	ParsedURI uri;
	std::string url = "http://workflow.polaris.service.b:8080?k1=v1#service.a";

	if (URIParser::parse(url, uri)) {
		fprintf(stderr, "Parse URI error.\n");
		wait_group.done();
		return;
	}

	pp->select(uri, NULL, &addr);

	if (addr)
		fprintf(stderr, "Select instance %s:%s\n",
				addr->host.c_str(), addr->port.c_str());
	else
		fprintf(stderr, "No instances match.\n");

	fprintf(stderr, "Press Ctrl-C to exit.\n");
}

void sig_handler(int signo) { wait_group.done(); }

int main(int argc, char *argv[]) {
	PolarisTask *task;

	if (argc != 2) {
		fprintf(stderr, "USAGE: %s <Polaris Cluster URL>\n", argv[0]);
		exit(1);
	}

	signal(SIGINT, sig_handler);

	std::string url = argv[1];
	if (strncasecmp(argv[1], "http://", 7) != 0 &&
		strncasecmp(argv[1], "https://", 8) != 0) {
		url = "http://" + url;
	}

	int ret = client.init(url);
	if (ret != 0) {
		client.deinit();
		exit(1);
	}
	task = client.create_discover_task("default", "workflow.polaris.service.b",
									   RETRY_MAX, polaris_callback);
	PolarisConfig config;
	task->set_config(std::move(config));
	task->start();

	wait_group.wait();
	delete pp;
	return 0;
}

