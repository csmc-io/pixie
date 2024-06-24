#!/bin/bash

# Copyright © 2024- Cosmic Observe, Inc.
# All Rights Reserved.
#
# NOTICE:  All information contained herein is, and remains
# the property of Cosmic Observe, Inc. and its suppliers,
# if any.
#
# SPDX-License-Identifier: Proprietary

# get directory of this script
config_dir="$(dirname "$0")"

kubectl -n plc-staging apply -f "${config_dir}/announcement_config.yaml"
kubectl -n plc-staging apply -f "${config_dir}/segment_config.yaml"
kubectl -n plc-staging apply -f "${config_dir}/sentry_config.yaml"
kubectl -n plc-staging apply -f "${config_dir}/auth_connector_config.yaml"
sops -d "${config_dir}/errors_config.yaml" | kubectl -n plc-staging apply -f -
sops -d "${config_dir}/ld_config.yaml" | kubectl -n plc-staging apply -f -
sops -d "${config_dir}/oauth_config.yaml" | kubectl -n plc-staging apply -f -
# sops -d "${config_dir}/segment_config.yaml" | kubectl -n plc-staging apply -f -
sops -d "${config_dir}/service_tls_certs.yaml" | kubectl -n plc-staging apply -f -
