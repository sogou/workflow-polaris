#include <signal.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include "PolarisManager.h"
#include "workflow/WFFacilities.h"

#define RETRY_MAX 5
#define REDIRECT_MAX 3

using namespace polaris;

static WFFacilities::WaitGroup main_wait_group(1);
static WFFacilities::WaitGroup query_wait_group(1);

void sig_handler(int signo)
{
	main_wait_group.done();
	query_wait_group.done();
}

static void get_task_addr(WFHttpTask *task, char **host_buf, size_t buf_len,
						  unsigned short *port)
{
	struct sockaddr_storage addr;
	socklen_t l = sizeof addr;
	task->get_peer_addr((struct sockaddr *)&addr, &l);

	if (addr.ss_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)&addr;
		inet_ntop(AF_INET, &sin->sin_addr, *host_buf, buf_len);
		*port = ntohs(sin->sin_port);
	} else if (addr.ss_family == AF_INET6) {
		struct sockaddr_in *sin = (struct sockaddr_in *)&addr;
		inet_ntop(AF_INET6, &sin->sin_addr, *host_buf, buf_len);
		*port = ntohs(sin->sin_port);
	}
}

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
					"http://default.workflow.polaris.service.b:8080"
					"#k1_env=v1_base&k2_number=v2_prime&a_namespace.a\n\n",
		argv[0], argv[0]);
		exit(1);
	}

	signal(SIGINT, sig_handler);

	std::string polaris_url = argv[1];
	std::string service_namespace = argv[2];
	std::string service_name = argv[3];

	if (strncasecmp(argv[1], "http://", 7) != 0 &&
		strncasecmp(argv[1], "https://", 8) != 0) {
		polaris_url = "http://" + polaris_url;
	}

	PolarisManager mgr(polaris_url);
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
			char addrstr[128];
			char *ptr = addrstr;
			unsigned short port = 0;

			get_task_addr(task, &ptr, 128, &port);
			fprintf(stderr, "Query task callback. state = %d error = %d "
					"addr = %s:%u\n", state, error, addrstr, port);

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

	fprintf(stderr, "Success. Make sure timer ends and press Ctrl-C to exit.\n");
	main_wait_group.wait();

	return 0;
}
