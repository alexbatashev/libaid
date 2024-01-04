"""Download external dependencies utils"""

load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)

def download_dependencies():
    """Download external dependencies"""

    http_archive(
        name = "liburing",
        urls = [
            "https://github.com/axboe/liburing/archive/refs/tags/liburing-2.5.tar.gz",
        ],
        strip_prefix = "liburing-liburing-2.5",
        sha256 = "456f5f882165630f0dc7b75e8fd53bd01a955d5d4720729b4323097e6e9f2a98",
        build_file_content = """filegroup(
            name = "srcs",
            srcs = glob(["**/*"]),
            visibility = ["//visibility:public"],
        )
        """,
    )
