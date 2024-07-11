#!/bin/bash

# Copyright © 2024–2026 Cosmic Observe, Inc.
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

# get directory of this script
config_dir="$(dirname "$0")"

kubectl -n plc apply -f "${config_dir}/announcement_config.yaml"
sops -d "${config_dir}/segment_config.yaml" | kubectl -n plc apply -f -
sops -d "${config_dir}/sentry_config.yaml" | kubectl -n plc apply -f -
kubectl -n plc apply -f "${config_dir}/auth_connector_config.yaml"
sops -d "${config_dir}/errors_config.yaml" | kubectl -n plc apply -f -
sops -d "${config_dir}/ld_config.yaml" | kubectl -n plc apply -f -
sops -d "${config_dir}/oauth_config.yaml" | kubectl -n plc apply -f -
sops -d "${config_dir}/service_tls_certs.yaml" | kubectl -n plc apply -f -
