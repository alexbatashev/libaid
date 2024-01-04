"""Extension file to download non-bzlmod dependencies"""

load("//utils/bazel:repositories.bzl", "download_dependencies")

def _init_repos_impl(_ctx):
    download_dependencies()

non_module_dependencies = module_extension(
    implementation = _init_repos_impl,
)
