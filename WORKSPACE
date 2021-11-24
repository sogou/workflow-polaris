workspace(name = "workflow-polaris")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "com_github_nlohmann_json",
    sha256 = "69cc88207ce91347ea530b227ff0776db82dcb8de6704e1a3d74f4841bc651cf",
    urls = [
		"https://github.com/nlohmann/json/releases/download/v3.6.1/include.zip",
    ],
    build_file = "//thirdparty:nlohmann_json.BUILD",
)

git_repository(
	name = "com_github_sogou_workflow",
	remote = "https://github.com/sogou/workflow.git",
	tag = "v0.9.8",
)
