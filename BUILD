load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test","cc_binary")

cc_library(
    name = 'workflow-polaris',
	hdrs = glob(["**/*.h", "**/*.hpp"]),
	srcs = glob(["src/*.cc"], exclude = ["**/*_test.cc"]),
    deps = [
		'@com_github_sogou_workflow//:http',
        '@com_github_nlohmann_json//:json',
    ],
    linkopts = [
        '-lpthread',
        '-lssl',
        '-lcrypto',
    ],
)
