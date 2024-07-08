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

#include "src/stirling/source_connectors/log_patterns/log_patterns_connector.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "src/common/base/base.h"
#include "src/shared/prometheus/decoder.h"

using px::shared::prometheus::Metric;
using px::shared::prometheus::DecodeMetric;
using px::shared::prometheus::FetchPrometheusMetrics;

namespace px {
namespace stirling {

Status LogPatternsConnector::InitImpl() {
  sampling_freq_mgr_.set_period(kSamplingPeriod);
  push_freq_mgr_.set_period(kPushPeriod);
  return Status::OK();
}

Status LogPatternsConnector::StopImpl() { return Status::OK(); }

void LogPatternsConnector::TransferLogPatternsTable(ConnectorContext* /*ctx*/,
                                                      DataTable* data_table) {

    int64_t timestamp = AdjustedSteadyClockNowNS();
    std::string prom_text = FetchPrometheusMetrics("http://node-agent-service.coroot.svc.cluster.local:80/metrics");
    LOG(INFO) << absl::Substitute("Fetched prom_text=$0", prom_text);
    std::vector<Metric> metrics = DecodeMetric(prom_text, "container_log_messages_total");
    for (auto& metric : metrics) {

        if (metric.labels["sample"] == "" ||
            metric.labels["pattern_hash"] == "" ||
            metric.labels["container_id"].find("pem")) {
            continue;
        }
        std::string str = ToString(metric);
        LOG(WARNING) << absl::Substitute("metric=$0", str);
        std::string pattern_hash = metric.labels["pattern_hash"];
        auto prev = pattern_counts_.find(pattern_hash);
        auto delta = metric.val;
        if (prev != pattern_counts_.end()) {
            delta = metric.val - prev->second;
        }
        pattern_counts_[pattern_hash] = metric.val;

        DataTable::RecordBuilder<&kLogPatternsTable> r(data_table, timestamp);
        r.Append<r.ColIndex("time_")>(timestamp);
        r.Append<r.ColIndex("container_id")>(metric.labels["container_id"]);
        r.Append<r.ColIndex("pattern_hash")>(pattern_hash);
        r.Append<r.ColIndex("source")>(metric.labels["source"]);
        r.Append<r.ColIndex("level")>(metric.labels["level"]);
        r.Append<r.ColIndex("message")>(metric.labels["sample"]);
        r.Append<r.ColIndex("count")>(delta);
    }
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
