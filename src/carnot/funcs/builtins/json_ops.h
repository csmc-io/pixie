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

#include <string>
#include <utility>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "src/carnot/udf/registry.h"
#include "src/carnot/udf/udf.h"

namespace px {
namespace carnot {
namespace builtins {

// TODO(zasgar): PL-419 To have proper support for JSON we need structs and nullable types.
// Revisit when we have them.
class PluckUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue in, StringValue key) {
    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(in.data());
    // TODO(zasgar/michellenguyen, PP-419): Replace with null when available.
    if (ok == nullptr) {
      return "";
    }
    if (!d.IsObject()) {
      return "";
    }
    if (!d.HasMember(key.data())) {
      return "";
    }
    const auto& plucked_value = d[key.data()];
    if (plucked_value.IsNull()) {
      return "";
    }
    if (plucked_value.IsString()) {
      return plucked_value.GetString();
    }

    // This is robust to nested JSON.
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    plucked_value.Accept(writer);
    return sb.GetString();
  }
};

class PluckAsInt64UDF : public udf::ScalarUDF {
 public:
  Int64Value Exec(FunctionContext*, StringValue in, StringValue key) {
    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(in.data());
    // TODO(zasgar/michellenguyen, PP-419): Replace with null when available.
    if (ok == nullptr) {
      return 0;
    }
    if (!d.IsObject()) {
      return 0;
    }
    if (!d.HasMember(key.data())) {
      return 0;
    }
    const auto& plucked_value = d[key.data()];
    if (plucked_value.IsNull()) {
      return 0;
    }
    if (plucked_value.IsInt64()) {
      return plucked_value.GetInt64();
    }
    return 0;
  }
};

class PluckAsFloat64UDF : public udf::ScalarUDF {
 public:
  Float64Value Exec(FunctionContext*, StringValue in, StringValue key) {
    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(in.data());
    // TODO(zasgar/michellenguyen, PP-419): Replace with null when available.
    if (ok == nullptr) {
      return 0.0;
    }
    if (!d.IsObject()) {
      return 0.0;
    }
    if (!d.HasMember(key.data())) {
      return 0.0;
    }
    const auto& plucked_value = d[key.data()];
    if (plucked_value.IsNull()) {
      return 0.0;
    }
    if (plucked_value.IsDouble()) {
      return plucked_value.GetDouble();
    }
    return 0.0;
  }
};

class PluckArrayUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue in, Int64Value index) {
    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(in.data());
    // TODO(zasgar/michellenguyen, PP-419): Replace with null when available.
    if (ok == nullptr) {
      return "";
    }
    if (!d.IsArray()) {
      return "";
    }
    const auto& plucked_array = d.GetArray();
    if (index < 0 || index >= plucked_array.Size()) {
      return "";
    }

    const auto& plucked_value = plucked_array[index.val];
    if (plucked_value.IsNull()) {
      return "";
    }
    if (plucked_value.IsString()) {
      return plucked_value.GetString();
    }

    // This is robust to nested JSON.
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    plucked_value.Accept(writer);
    return sb.GetString();
  }
};

class SplitUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue in, StringValue delimiter) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartArray();

    for (absl::string_view part : absl::StrSplit(in.data(), delimiter.data())) {
      writer.String(part.data(), part.size());
    }

    writer.EndArray();
    return sb.GetString();
  }

};

/**
  DocString intentionally omitted, this is a non-public function.
  This function creates a custom deep link by creating a "script reference" from a label,
  script name, and input script arguments. The compiler translates the public API into this UDF,
  and the public API will be documented in the compile time functions.

  ScriptReferenceUDF takes in a label, script, and set of variadic script args.
  These script args passed in the alternating form argname0, argval0, argname1., argval1.
  Since script args are always expressed as strings in vis specs, these arg values are
  also passed in as strings. (When a script is executed, its script args are parsed by
  the compiler into their proper data type).
 */
template <typename... T>
class ScriptReferenceUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue label, StringValue script, T... args) {
    return ExecImpl(label, script, {std::forward<T>(args)...});
  }

  // Some hacky stuff due to variadic args....
  static udf::InfRuleVec SemanticInferenceRules() {
    const std::size_t num_script_args = sizeof...(T);
    auto num_input_args = num_script_args + 2;  // add 1 each for script and label.
    std::vector<types::SemanticType> input_st(num_input_args, types::SemanticType::ST_NONE);

    return {udf::ExplicitRule::Create<ScriptReferenceUDF<T...>>(
        types::SemanticType::ST_SCRIPT_REFERENCE, input_st)};
  }

 private:
  StringValue ExecImpl(StringValue label, StringValue script,
                       std::initializer_list<StringValue> values) {
    rapidjson::Document d;
    d.SetObject();
    d.AddMember("label", rapidjson::Value().SetString(label.c_str(), d.GetAllocator()).Move(),
                d.GetAllocator());
    d.AddMember("script", rapidjson::Value().SetString(script.c_str(), d.GetAllocator()).Move(),
                d.GetAllocator());

    // Construct the args object
    rapidjson::Value argsObj;
    argsObj.SetObject();

    std::string arg_name;
    int32_t counter = 0;
    for (const auto& arg_val : values) {
      if (counter % 2) {
        argsObj.AddMember(rapidjson::Value().SetString(arg_name.c_str(), d.GetAllocator()).Move(),
                          rapidjson::Value().SetString(arg_val.c_str(), d.GetAllocator()).Move(),
                          d.GetAllocator());
      } else {
        arg_name = arg_val;
      }
      counter++;
    }
    d.AddMember("args", argsObj.Move(), d.GetAllocator());

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    d.Accept(writer);
    return sb.GetString();
  }
};

void RegisterJSONOpsOrDie(udf::Registry* registry);

}  // namespace builtins
}  // namespace carnot
}  // namespace px
