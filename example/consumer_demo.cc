#include <unistd.h>
#include <string>
#include "PolarisManager.h"
#include "workflow/WFFacilities.h"

using namespace polaris;

WFFacilities::WaitGroup wait_group(1);

int main(int argc, char *argv[])
{
	if (argc != 5) {
		fprintf(stderr, "USAGE:\n\t%s <polaris cluster> "
					"<namespace> <service_name> <query URL>\n\n"
				"QUERY URL FORMAT:\n"
					"\thttp://callee_service_namespace.callee_service_name:port"
					"#k1=v1&caller_service_namespace.caller_service_name\n\n"
				"EXAMPLE:\n\t%s http://127.0.0.1:8090 "
					"default workflow.polaris.service.b "
					"\"http://default.workflow.polaris.service.b:8080"
					"#k1_env=v1_base&k2_number=v2_prime&a_namespace.a\"\n\n",
				argv[0], argv[0]);
		exit(1);
	}

	std::string polaris_url = argv[1];
	std::string service_namespace = argv[2];
	std::string service_name = argv[3];
	const char *query_url = argv[4];

	if (strncasecmp(argv[1], "http://", 7) != 0 &&
		strncasecmp(argv[1], "https://", 8) != 0) {
		polaris_url = "http://" + polaris_url;
	}

	// 1. Construct PolarisManager.
	PolarisManager mgr(polaris_url);

	// 2. Watch a service, will fill Workflow with instances automatically.
	int ret = mgr.watch_service(service_namespace, service_name);

	fprintf(stderr, "Watch %s %s ret=%d.\n", service_namespace.c_str(),
			service_name.c_str(), ret);

	if (ret < 0)
	{
		fprintf(stderr, "Watch failed. error=%d\n", mgr.get_error());
		return 0;
	}

	// 3. Use Workflow as usual. Tasks will be send to the right instances.
	fprintf(stderr, "Query URL : %s\n", query_url);
	WFHttpTask *task = WFTaskFactory::create_http_task(query_url,
													   3, /* REDIRECT_MAX */
													   5, /* RETRY_MAX */
													   [](WFHttpTask *task) {
		fprintf(stderr, "Query callback. state = %d error = %d\n",
				task->get_state(), task->get_error());
		wait_group.done();
	});

	task->start();
	wait_group.wait();

	// 4. Unwatch the service when existing the progress.
	ret = mgr.unwatch_service(service_namespace, service_name);
	fprintf(stderr, "Unwatch %s %s ret=%d.\n", service_namespace.c_str(),
			service_name.c_str(), ret);

	return 0;
}
