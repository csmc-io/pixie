# Copyright 2018- The Pixie Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

load("@px//bazel:llvm_cmake.bzl", "add_llvm_cache_entries", "llvm_build_data_deps")
load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

licenses(["notice"])

cc_library(
    name = "cpr",
    hdrs = glob(["include/cpr/**/*.h"]),
    includes = [
        "include"
    ],
    srcs = glob(["cpr/**/*.cpp"]),
    visibility = ["//visibility:public"],
    deps = [
        "@curl//:curl",
    ],
)
