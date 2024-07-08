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
static constexpr DataElement kLogPatternsElements[] = {
        canonical_data_elements::kTime,
        /* canonical_data_elements::kUPID, */
        {"container_id", "ID of the container", types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
        {"pattern_hash", "Hash of the log pattern", types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
        {"source", "Source of the log message", types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
        {"level", "Log level of the message", types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
        {"message", "Sample of the log message", types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
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
