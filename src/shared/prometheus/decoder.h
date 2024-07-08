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

#include "src/common/base/base.h"

namespace px {
namespace shared {
namespace prometheus {

struct Metric {
  std::string name;
  std::map<std::string, std::string> labels;
  uint64_t val;
};

inline std::string ToString(const Metric& metric) {
  std::string str = absl::StrCat(metric.name, " ",  metric.val);
  for (const auto& [key, val] : metric.labels) {
    absl::StrAppend(&str, " ", key, "=", val);
  }
  return str;
}

inline std::string ToString(const std::vector<Metric> metrics) {
    std::string str;
    for (const auto& metric : metrics) {
        absl::StrAppend(&str, ToString(metric), "\n");
    }
    return str;
}

// print metric to a string

std::vector<Metric> DecodeMetric(const std::string& prom_text, const std::string& metric_name);

std::string FetchPrometheusMetrics(const std::string& url);

}  // namespace prometheus
}  // namespace shared
}  // namespace px
