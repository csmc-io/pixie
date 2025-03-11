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

#include <absl/strings/strip.h>
#include <algorithm>
#include <string>
#include "src/carnot/udf/registry.h"
#include "src/common/base/utils.h"
#include "src/shared/types/types.h"

namespace px {
namespace carnot {
namespace builtins {

class ContainsUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, StringValue b1, StringValue b2) {
    return absl::StrContains(b1, b2);
  }

};

class LengthUDF : public udf::ScalarUDF {
 public:
  Int64Value Exec(FunctionContext*, StringValue b1) { return b1.length(); }
};

class FindUDF : public udf::ScalarUDF {
 public:
  Int64Value Exec(FunctionContext*, StringValue src, StringValue substr) {
    return src.find(substr);
  }

};

class SubstringUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue b1, Int64Value pos, Int64Value length) {
    // If the pos is "erroneous" then just return empty string.
    if (pos < 0 || pos > static_cast<int64_t>(b1.length()) || length < 0) {
      return "";
    }
    return b1.substr(static_cast<size_t>(pos.val), static_cast<size_t>(length.val));
  }
};

class ToLowerUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue b1) {
    transform(b1.begin(), b1.end(), b1.begin(), ::tolower);
    return b1;
  }
};

class ToUpperUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue b1) {
    transform(b1.begin(), b1.end(), b1.begin(), ::toupper);
    return b1;
  }
};

class TrimUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue s) {
    std::string val = s;
    absl::StripAsciiWhitespace(&val);
    return val;
  }
};

class StripPrefixUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue prefix, StringValue s) {
    return StringValue(absl::StripPrefix(s, prefix));
  }
};

class HexToASCII : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue h) {
    std::string result;
    auto s_or_res = AsciiHexToBytes<std::string>(h);
    if (s_or_res.ok()) {
      return s_or_res.ConsumeValueOrDie();
    }
    return "";
  }

};

class BytesToHex : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue h) { return BytesToString<bytes_format::Hex>(h); }

};

class StringToIntUDF : public udf::ScalarUDF {
 public:
  Int64Value Exec(FunctionContext*, StringValue input, Int64Value default_val) {
    int64_t val;
    if (!absl::SimpleAtoi(input, &val)) {
      return default_val;
    }
    return val;
  }
};

class IntToStringUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Int64Value input) { return std::to_string(input.val); }
};

void RegisterStringOpsOrDie(udf::Registry* registry);

}  // namespace builtins
}  // namespace carnot
}  // namespace px
