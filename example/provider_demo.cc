#include <signal.h>
#include <unistd.h>
#include <string>
#include "PolarisManager.h"
#include "workflow/WFFacilities.h"
#include "workflow/WFHttpServer.h"

using namespace polaris;

static WFFacilities::WaitGroup register_wait_group(1);
static WFFacilities::WaitGroup deregister_wait_group(1);

void sig_handler(int signo)
{
	register_wait_group.done();
	deregister_wait_group.done();
}

int main(int argc, char *argv[])
{
	if (argc != 6) {
		fprintf(stderr, "USAGE:\n\t%s <polaris cluster> "
				"<namespace> <service_name> <localhost> <port>\n\n", argv[0]);
		exit(1);
	}

	signal(SIGINT, sig_handler);

	std::string polaris_url = argv[1];
	std::string service_namespace = argv[2];
	std::string service_name = argv[3];
	std::string host = argv[4];
	std::string port = argv[5];

	if (strncasecmp(argv[1], "http://", 7) != 0 &&
		strncasecmp(argv[1], "https://", 8) != 0) {
		polaris_url = "http://" + polaris_url;
	}

	WFHttpServer server([port](WFHttpTask *task) {
		task->get_resp()->append_output_body(
				"Response from instance 127.0.0.1:" + port);
	});

	if (server.start(atoi(port.c_str())) != 0) {
		fprintf(stderr, "start server error\n");
		return 0;
	}

	PolarisManager mgr(polaris_url);

	PolarisInstance instance;
	instance.set_host(host);
	instance.set_port(atoi(port.c_str()));
	std::map<std::string, std::string> meta = {{"key1", "value1"}};
	instance.set_metadata(meta);
	instance.set_region("south-china");
	instance.set_zone("shenzhen");

	int ret = mgr.register_service(service_namespace, service_name,
								   std::move(instance));
	fprintf(stderr, "Register %s %s %s %s ret=%d.\n",
			service_namespace.c_str(), service_name.c_str(),
			host.c_str(), port.c_str(), ret);

	fprintf(stderr, "Success. Press Ctrl-C to exit.\n");
	register_wait_group.wait();

	if (ret == 0) {
		bool deregister_ret = mgr.deregister_service(service_namespace,
													 service_name,
													 std::move(instance));
		fprintf(stderr, "Deregister %s %s %s %s ret=%d.\n",
				service_namespace.c_str(), service_name.c_str(),
				host.c_str(), port.c_str(), deregister_ret);
		deregister_wait_group.wait();
	}

	server.stop();
	return 0;
}
