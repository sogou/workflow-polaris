cc_binary(
	name = 'consumer_demo',
	srcs = ['consumer_demo.cc'],
	copts = ['-Isrc/'],
	deps = [
		'//:workflow-polaris',
		'@com_github_sogou_workflow//:http',
		'@com_github_sogou_workflow//:upstream',
		'@com_github_sogou_workflow//:workflow_hdrs',
	],
)

cc_binary(
	name = 'provider_demo',
	srcs = ['provider_demo.cc'],
	copts = ['-Isrc/'],
	deps = [
		'//:workflow-polaris',
		'@com_github_sogou_workflow//:http',
		'@com_github_sogou_workflow//:upstream',
		'@com_github_sogou_workflow//:workflow_hdrs',
	],
)

cc_binary(
	name = 'local_test',
	srcs = ['local_test.cc'],
	copts = ['-Isrc/'],
	deps = [
		'//:workflow-polaris',
		'@com_github_sogou_workflow//:http',
		'@com_github_sogou_workflow//:upstream',
		'@com_github_sogou_workflow//:workflow_hdrs',
	],
)
