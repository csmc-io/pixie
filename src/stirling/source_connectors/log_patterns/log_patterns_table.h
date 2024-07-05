/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/common/base/base.h"
#include "src/common/system/system.h"
#include "src/shared/metadata/metadata.h"
#include "src/stirling/core/canonical_types.h"
#include "src/stirling/core/output.h"
#include "src/stirling/core/source_connector.h"

namespace px {
namespace stirling {

// clang-format off
// container_log_messages_total{container_id="/k8s/kube-system/kube-dns-bd76ffc78-68wdb/kubedns-metrics-collector",level="error",machine_id="c605c971c9d0a43aa7618dd79e5b2500",pattern_hash="d41d8cd98f00b204e9800998ecf8427e",sample="{\"level\":\"error\",\"ts\":1720186648.5371022,\"caller\":\"gcm/export.go:498\",\"msg\":\"Failed to export self-observability metrics to Cloud Monitoring\",\"error\":\"rpc error: code = PermissionDenied desc = Permission monitoring.timeSeries.create denied (or the resource may not exist).\",\"stacktrace\":\"google3/cloud/kubernetes/metrics/common/gcm/gcm.(*exporter).startSelfObservability\\n\\tcloud/kubernetes/metrics/common/gcm/export.go:498\"}",source="stdout/stderr",system_uuid="c605c971-c9d0-a43a-a761-8dd79e5b2500"} 18
static constexpr DataElement kLogPatternsElements[] = {
        canonical_data_elements::kTime,
        /* canonical_data_elements::kUPID, */
        {"container_id", "ID of the container", types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
        {"pattern_hash", "Hash of the log pattern", types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
        {"source", "Source of the log message", types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
        {"level", "Log level of the message", types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
        {"message", "Log message", types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
        {"count", "Number of occurrences of the log pattern", types::DataType::INT64, types::SemanticType::ST_NONE, types::PatternType::METRIC_COUNTER},
};

constexpr DataTableSchema kLogPatternsTable(
    "log_patterns",
    "Log patterns detected by the agent",
    kLogPatternsElements
);
// clang-format on
DEFINE_PRINT_TABLE(LogPatterns)

// TODO(oazizi): Enable version below, once rest of the agent supports tabletization.
//               Can't enable yet because it would result in time-scrambling.
//  static constexpr std::string_view kLogPatternsTabletizationKey = "upid";
//  static constexpr auto kLogPatternsTable =
//      DataTableSchema("process_stats", "CPU, memory and IO metrics for processes",
//      kLogPatternsElements, kLogPatternsTabletizationKey);

}  // namespace stirling
}  // namespace px
