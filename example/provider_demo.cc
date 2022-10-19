#include <unistd.h>
#include <string>
#include "PolarisManager.h"
#include "workflow/WFHttpServer.h"

using namespace polaris;

int main(int argc, char *argv[])
{
	if (argc < 6 || argc > 9) {
		fprintf(stderr, "USAGE:\n\t%s <polaris cluster> "
				"<namespace> <service_name> <localhost> <port> "
				"[<service_token>] [<platform_id>] [<platform_token>]\n\n",
				argv[0]);
		exit(1);
	}

	std::string polaris_url = argv[1];
	std::string service_namespace = argv[2];
	std::string service_name = argv[3];
	std::string host = argv[4];
	std::string port = argv[5];
	std::string service_token;
	std::string platform_id;
	std::string platform_token;

	if (argc >= 7)
		service_token = argv[6];
	if (argc >= 8)
		platform_id = argv[7];
	if (argc == 9)
		platform_token = argv[8];

	if (strncasecmp(argv[1], "http://", 7) != 0 &&
		strncasecmp(argv[1], "https://", 8) != 0) {
		polaris_url = "http://" + polaris_url;
	}

	// Prepare: Start a server for test.
	WFHttpServer server([port](WFHttpTask *task) {
		fprintf(stderr, "Test server get request.\n");
		task->get_resp()->append_output_body(
				"Response from instance 127.0.0.1:" + port);
	});

	if (server.start(atoi(port.c_str())) != 0) {
		fprintf(stderr, "start server error\n");
		return 0;
	}

	// 1. Construct PolarisManager with polaris_url.
//	PolarisManager mgr(polaris_url);
	PolarisManager mgr(polaris_url, platform_id, platform_token, "polaris.yaml.template");

	int healthcheck_ttl = 2;
	int heartbeat_interval = 1;

	PolarisInstance instance;
	instance.set_host(host);
	instance.set_port(atoi(port.c_str()));
	std::map<std::string, std::string> meta = {{"key1", "value1"}};
	instance.set_metadata(meta);
	instance.set_region("north-china");
	instance.set_zone("beijing");
	instance.set_campus("wudaokou");
	instance.set_enable_healthcheck(true); // default : false
	instance.set_healthcheck_ttl(healthcheck_ttl); // default : 5s

	// 2. Register instance.
	int ret = mgr.register_service(service_namespace, service_name, service_token,
								   heartbeat_interval, instance);
	fprintf(stderr, "Register %s %s %s %s ret=%d.\n",
			service_namespace.c_str(), service_name.c_str(),
			host.c_str(), port.c_str(), ret);

	if (ret >= 0) {
		fprintf(stderr, "Success. Press \"Enter\" to deregister.\n");
		getchar();

		// 3. Deregister instance.
		ret = mgr.deregister_service(service_namespace, service_name,
									 service_token, std::move(instance));
		fprintf(stderr, "Deregister %s %s %s %s ret=%d.\n",
				service_namespace.c_str(), service_name.c_str(),
				host.c_str(), port.c_str(), ret);
	} else {
		fprintf(stderr, "Register failed. error=%d\n", mgr.get_error());
	}

	server.stop();
	return 0;
}
