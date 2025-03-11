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

#include <rapidjson/document.h>

#include <absl/strings/strip.h>

#include <algorithm>
#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <vector>
#include "re2/re2.h"
#include "src/carnot/udf/registry.h"
#include "src/common/base/utils.h"
#include "src/shared/types/types.h"

namespace px {
namespace carnot {
namespace builtins {

class RegexMatchUDF : public udf::ScalarUDF {
 public:
  Status Init(FunctionContext*, StringValue regex) {
    re2::RE2::Options opts;
    opts.set_dot_nl(true);
    opts.set_log_errors(false);
    regex_ = std::make_unique<re2::RE2>(regex, opts);
    return Status::OK();
  }
  BoolValue Exec(FunctionContext*, StringValue input) {
    if (regex_->error_code() != RE2::NoError) {
      return false;
    }
    return RE2::FullMatch(input, *regex_);
  }

 private:
  std::unique_ptr<re2::RE2> regex_;
};

class RegexReplaceUDF : public udf::ScalarUDF {
 public:
  Status Init(FunctionContext*, StringValue regex_pattern) {
    re2::RE2::Options opts;
    opts.set_log_errors(false);
    regex_ = std::make_unique<re2::RE2>(regex_pattern, opts);
    return Status::OK();
  }
  StringValue Exec(FunctionContext*, StringValue input, StringValue sub) {
    if (regex_->error_code() != RE2::NoError) {
      return absl::Substitute("Invalid regex expr: $0", regex_->error());
    }
    std::string err_str;
    if (!regex_->CheckRewriteString(sub, &err_str)) {
      return absl::Substitute("Invalid regex in substitution string: $0", err_str);
    }
    RE2::GlobalReplace(&input, *regex_, sub);
    return input;
  }


 private:
  std::unique_ptr<re2::RE2> regex_;
};

class MatchRegexRule : public udf::ScalarUDF {
 public:
  Status Init(FunctionContext* ctx, StringValue encodedRegexRules) {
    // Parse encodedRegexRules as json.
    rapidjson::Document regex_rules_json;
    rapidjson::ParseResult parse_result = regex_rules_json.Parse(encodedRegexRules.data());
    if (!parse_result) {
      return Status(statuspb::Code::INVALID_ARGUMENT, "unable to parse string as json");
    }
    // Populate the parse regular expressions into self::regex_rules.
    for (rapidjson::Value::ConstMemberIterator itr = regex_rules_json.MemberBegin();
         itr != regex_rules_json.MemberEnd(); ++itr) {
      RegexMatchUDF regex_match_udf;
      std::string name = itr->name.GetString();
      std::string regex_pattern = itr->value.GetString();
      PX_RETURN_IF_ERROR(regex_match_udf.Init(ctx, regex_pattern));
      regex_rules.emplace_back(make_pair(name, std::move(regex_match_udf)));
      regex_rules_length++;
    }
    return Status::OK();
  }

  types::StringValue Exec(FunctionContext* ctx, StringValue value) {
    for (int i = 0; i < regex_rules_length; i++) {
      if (regex_rules[i].second.Exec(ctx, value).val) {
        return regex_rules[i].first;
      }
    }
    return "";
  }


 private:
  int regex_rules_length = 0;
  std::vector<std::pair<std::string, RegexMatchUDF> > regex_rules;
};

void RegisterRegexOpsOrDie(udf::Registry* registry);

}  // namespace builtins
}  // namespace carnot
}  // namespace px
