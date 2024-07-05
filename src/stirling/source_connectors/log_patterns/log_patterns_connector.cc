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

#include "src/stirling/source_connectors/process_stats/process_stats_connector.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "src/common/base/base.h"

namespace px {
namespace stirling {

Status LogPatternsConnector::InitImpl() {
  sampling_freq_mgr_.set_period(kSamplingPeriod);
  push_freq_mgr_.set_period(kPushPeriod);
  return Status::OK();
}

Status LogPatternsConnector::StopImpl() { return Status::OK(); }

void LogPatternsConnector::TransferLogPatternsTable(ConnectorContext* ctx,
                                                      DataTable* data_table) {
}

void LogPatternsConnector::TransferDataImpl(ConnectorContext* ctx) {
  DCHECK_EQ(data_tables_.size(), 1U);

  auto* data_table = data_tables_[0];

  if (data_table == nullptr) {
    return;
  }

  TransferLogPatternsTable(ctx, data_table);
}

}  // namespace stirling
}  // namespace px
