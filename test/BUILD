cc_test(
	name = 'unittest',
	srcs = ['polaris_policy_unittest.cc'],
    copts = ['-Iexternal/gtest/include'],
	deps = [
		'//:workflow-polaris',
		'@com_github_sogou_workflow//:http',
		'@com_github_sogou_workflow//:upstream',
		'@com_github_sogou_workflow//:workflow_hdrs',
		'@com_google_googletest//:gtest',
		'@com_google_googletest//:gtest_main',
	],
)

