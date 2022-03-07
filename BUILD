load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test","cc_binary")

cc_library(
	name = 'workflow-polaris',
	hdrs = glob(["**/*.h", "**/*.hpp"]),
	srcs = glob(["src/*.cc"], exclude = ["**/*_test.cc"]),
	deps = [
		'@com_github_sogou_workflow//:http',
		'@com_github_sogou_workflow//:upstream',
		"@com_github_jbeder_yaml_cpp//:yaml-cpp",
	],
	linkopts = [
		'-lpthread',
		'-lssl',
		'-lcrypto',
	],
	visibility = ["//visibility:public"]
)

