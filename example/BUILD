cc_binary(
	name = 'demo',
	srcs = ['demo.cc'],
	copts = ['-Isrc/'],
	deps = [
		'//:workflow-polaris',
		'@com_github_sogou_workflow//:http',
		'@com_github_sogou_workflow//:upstream',
		'@com_github_sogou_workflow//:workflow_hdrs',
	],
)

cc_binary(
	name = 'local_demo',
	srcs = ['local_demo.cc'],
	copts = ['-Isrc/'],
	deps = [
		'//:workflow-polaris',
		'@com_github_sogou_workflow//:http',
		'@com_github_sogou_workflow//:upstream',
		'@com_github_sogou_workflow//:workflow_hdrs',
	],
)

