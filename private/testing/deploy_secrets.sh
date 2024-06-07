#!/bin/bash

# get directory of this script
config_dir="$(dirname "$0")"


kubectl -n plc-testing apply -f "${config_dir}/announcement_config.yaml"
kubectl -n plc-testing apply -f "${config_dir}/segment_config.yaml"
kubectl -n plc-testing apply -f "${config_dir}/sentry_config.yaml"
kubectl -n plc-testing apply -f "${config_dir}/errors_config.yaml"
kubectl -n plc-testing apply -f "${config_dir}/auth_connector_config.yaml"
sops -d "${config_dir}/ld_config.yaml" | kubectl -n plc-testing apply -f -
sops -d "${config_dir}/oauth_config.yaml" | kubectl -n plc-testing apply -f -
sops -d "${config_dir}/service_tls_certs.yaml" | kubectl -n plc-testing apply -f -
