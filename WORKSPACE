workspace(name = "workflow-polaris")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
	name = "com_github_sogou_workflow",
	remote = "https://github.com/sogou/workflow.git",
	tag = "v0.10.7",
)

http_archive(
    name = "bazel_skylib",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
    ],
    sha256 = "74d544d96f4a5bb630d465ca8bbcfe231e3594e5aae57e1edbf17a6eb3ca2506",
)

http_archive(
    name = "com_google_googletest",
    url = "https://github.com/google/googletest/archive/release-1.11.0.zip",
	strip_prefix = "googletest-release-1.11.0",
)

git_repository(
	name = "com_github_jbeder_yaml_cpp",
	remote = "https://github.com/jbeder/yaml-cpp.git",
	tag = "yaml-cpp-0.7.0",
)

