#include <signal.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include "PolarisManager.h"
#include "workflow/WFFacilities.h"
#include "workflow/WFHttpServer.h"

#define RETRY_MAX 5
#define REDIRECT_MAX 3
#define INSTANCES_NUM 4

using namespace polaris;

static WFFacilities::WaitGroup main_wait_group(1);
static WFFacilities::WaitGroup query_wait_group(1);
static const std::string service_namespace = "default";
static const std::string service_name = "workflow.polaris.service.b";
WFHttpServer *instances[INSTANCES_NUM];

void sig_handler(int signo)
{
	main_wait_group.done();
	query_wait_group.done();
}

void start_test_servers()
{
	for (size_t i = 0; i < INSTANCES_NUM; i++)
	{
		std::string port = "800";
		port += std::to_string(i);

		instances[i] = new WFHttpServer([port](WFHttpTask *task) {
			task->get_resp()->append_output_body(
						"Response from instance 127.0.0.0.1:" + port);
		});

		if (instances[i]->start(atoi(port.c_str())) != 0) {
			fprintf(stderr, "Start instance 127.0.0.0.1:%s failed.\n",
					port.c_str());
			delete instances[i];
			instances[i] = NULL;
		}
	}
}

void stop_test_servers()
{
	for (size_t i = 0; i < INSTANCES_NUM; i++)
	{
		if (instances[i])
		{
			instances[i]->stop();
			delete instances[i];
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s <Polaris Cluster URL>\n", argv[0]);
		exit(1);
	}

	signal(SIGINT, sig_handler);

	start_test_servers();

	std::string polaris_url = argv[1];
	if (strncasecmp(argv[1], "http://", 7) != 0 &&
		strncasecmp(argv[1], "https://", 8) != 0) {
		polaris_url = "http://" + polaris_url;
	}

	PolarisManager mgr(polaris_url, "./polaris.yaml.template");
	int ret = mgr.watch_service(service_namespace, service_name);
	fprintf(stderr, "Watch %s %s ret=%d.\n", service_namespace.c_str(),
			service_name.c_str(), ret);
	if (ret)
		return 0;

	std::string url = "http://" + service_namespace + "." + service_name +
					  ":8080#k1_env=v1_base&k2_number=v2_prime&a_namespace.a";
	fprintf(stderr, "URL : %s\n", url.c_str());

	WFHttpTask *task = WFTaskFactory::create_http_task(url,
													   REDIRECT_MAX,
													   RETRY_MAX,
													   [](WFHttpTask *task) {
			int state = task->get_state();
			int error = task->get_error();
			fprintf(stderr, "Query task callback. state = %d error = %d\n",
					state, error);

			if (state == WFT_STATE_SUCCESS) {
				const void *body;
				size_t body_len;
				task->get_resp()->get_parsed_body(&body, &body_len);
				fwrite(body, 1, body_len, stdout);
				fflush(stdout);
			}

			query_wait_group.done();
	});

	task->start();
	query_wait_group.wait();

	bool unwatch_ret = mgr.unwatch_service(service_namespace, service_name);
	fprintf(stderr, "\nUnwatch %s %s ret=%d.\n", service_namespace.c_str(),
			service_name.c_str(), unwatch_ret);

	stop_test_servers();
	fprintf(stderr, "Success. Make sure the timer ends and press Ctrl-C to exit.\n");
	main_wait_group.wait();

	return 0;
}
