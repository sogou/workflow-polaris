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
	EndpointAddress *addr;
	ParsedURI uri;
	std::string url = "http://workflow.polaris.service.b:8080?k1_env=v1_base&k2_number=v2_prime#a";

	if (URIParser::parse(url, uri)) {
		fprintf(stderr, "Parse URI error.\n");
		wait_group.done();
		return;
	}
	fprintf(stderr, "uri.caller=%s\n", uri.fragment);
	pp->select(uri, NULL, &addr);
	fprintf(stderr, "select %s:%s\n", addr->host.c_str(), addr->port.c_str());
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

	const char *service_name = discover.service_name.c_str();
	fprintf(stderr, "\nDiscover task success. service_name = %s\n",
			service_name);

	WFNameService *ns = WFGlobal::get_name_service();
	PolarisPolicyConfig conf(service_name);
	pp = new PolarisPolicy(&conf);

	if (ns->add_policy(service_name, pp) < 0) {
		fprintf(stderr, "Policy %s existed.\n", service_name);
		delete pp;
		pp = static_cast<PolarisPolicy *>(ns->get_policy(service_name));
	}
	else
		fprintf(stderr, "Successfully add PolarisPolicy %s\n", service_name);

	if (discover.instances.size())
		pp->update_instances(discover.instances);

	if (route.routing_inbounds.size())
		pp->update_inbounds(route.routing_inbounds);

	if (route.routing_outbounds.size())
		pp->update_outbounds(route.routing_outbounds);

	query();
	
	fprintf(stderr, "\nFinish. Press Ctrl-C to exit.\n");
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

