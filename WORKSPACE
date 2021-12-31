workspace(name = "workflow-polaris")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
	name = "com_github_sogou_workflow",
	remote = "https://github.com/sogou/workflow.git",
	tag = "v0.9.9",
)

http_archive(
    name = "com_google_googletest",
    url = "https://github.com/google/googletest/archive/release-1.10.0.zip",
	strip_prefix = "googletest-release-1.10.0",
)

