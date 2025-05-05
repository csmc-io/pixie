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
#include "src/stirling/core/canonical_types.h"
#include "src/stirling/core/source_connector.h"
#include "src/stirling/source_connectors/log_patterns/log_patterns_table.h"

namespace px {
namespace stirling {

class LogPatternsConnector : public SourceConnector {
 public:
  static constexpr std::string_view kName = "log_patterns";
  static constexpr auto kSamplingPeriod = std::chrono::milliseconds{1000};
  static constexpr auto kPushPeriod = std::chrono::milliseconds{1000};
  static constexpr auto kTables = MakeArray(kLogPatternsTable);
  static constexpr uint32_t kLogPatternsTableNum = TableNum(kTables, kLogPatternsTable);

  std::map<std::string, uint64_t> pattern_counts_;

  LogPatternsConnector() = delete;
  ~LogPatternsConnector() override = default;

  static std::unique_ptr<SourceConnector> Create(std::string_view name) {
    return std::unique_ptr<SourceConnector>(new LogPatternsConnector(name));
  }

  Status InitImpl() override;

  Status StopImpl() override;

  void TransferDataImpl(ConnectorContext* ctx) override;

 protected:
  explicit LogPatternsConnector(std::string_view source_name)
      : SourceConnector(source_name, kTables) {}

 private:
  void TransferLogPatternsTable(ConnectorContext* ctx, DataTable* data_table);
};

}  // namespace stirling
}  // namespace px
