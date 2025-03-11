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

#include <cmath>
#include <limits>

#include "src/carnot/udf/registry.h"
#include "src/carnot/udf/type_inference.h"
#include "src/shared/types/types.h"
#include "src/shared/types/typespb/types.pb.h"

namespace px {
namespace carnot {
namespace builtins {

template <typename TReturn, typename TArg1 = TReturn, typename TArg2 = TReturn>
class AddUDF : public udf::ScalarUDF {
 public:
  TReturn Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1.val + b2.val; }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {
        udf::InheritTypeFromArgs<AddUDF>::Create({types::ST_BYTES, types::ST_THROUGHPUT_PER_NS,
                                                  types::ST_THROUGHPUT_BYTES_PER_NS,
                                                  types::ST_DURATION_NS}),
    };
  }
};

template <>
class AddUDF<types::StringValue> : public udf::ScalarUDF {
 public:
  types::StringValue Exec(FunctionContext*, types::StringValue b1, types::StringValue b2) {
    return b1 + b2;
  }

};

template <typename TReturn, typename TArg1 = TReturn, typename TArg2 = TReturn>
class SubtractUDF : public udf::ScalarUDF {
 public:
  TReturn Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1.val - b2.val; }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {
        udf::InheritTypeFromArgs<SubtractUDF>::Create({types::ST_BYTES, types::ST_THROUGHPUT_PER_NS,
                                                       types::ST_THROUGHPUT_BYTES_PER_NS,
                                                       types::ST_DURATION_NS}),
    };
  }
};

template <typename TArg1, typename TArg2 = TArg1>
class DivideUDF : public udf::ScalarUDF {
 public:
  types::Float64Value Exec(FunctionContext*, TArg1 b1, TArg2 b2) {
    return static_cast<double>(b1.val) / static_cast<double>(b2.val);
  }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<DivideUDF>(types::ST_THROUGHPUT_PER_NS,
                                                 {types::ST_NONE, types::ST_DURATION_NS}),
            udf::ExplicitRule::Create<DivideUDF>(types::ST_THROUGHPUT_BYTES_PER_NS,
                                                 {types::ST_BYTES, types::ST_DURATION_NS})};
  }

};

template <typename TReturn, typename TArg1 = TReturn, typename TArg2 = TReturn>
class MultiplyUDF : public udf::ScalarUDF {
 public:
  TReturn Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1.val * b2.val; }
};

template <typename TBase, typename TVal>
class LogUDF : public udf::ScalarUDF {
 public:
  Float64Value Exec(FunctionContext*, TBase b, TVal v) {
    return log(static_cast<double>(v.val)) / log(static_cast<double>(b.val));
  }
};

template <typename TArg1>
class LnUDF : public udf::ScalarUDF {
 public:
  Float64Value Exec(FunctionContext*, TArg1 v) { return log(static_cast<double>(v.val)); }
};

template <typename TArg1>
class Log2UDF : public udf::ScalarUDF {
 public:
  Float64Value Exec(FunctionContext*, TArg1 v) { return log2(static_cast<double>(v.val)); }
};

template <typename TArg1>
class Log10UDF : public udf::ScalarUDF {
 public:
  Float64Value Exec(FunctionContext*, TArg1 v) { return log10(static_cast<double>(v.val)); }
};
template <typename TArg1, typename TArg2>
class PowUDF : public udf::ScalarUDF {
 public:
  Float64Value Exec(FunctionContext*, TArg1 x, TArg2 y) {
    return pow(static_cast<double>(x.val), static_cast<double>(y.val));
  }
};

template <typename TArg1>
class ExpUDF : public udf::ScalarUDF {
 public:
  Float64Value Exec(FunctionContext*, TArg1 y) { return exp(static_cast<double>(y.val)); }

};

template <typename TArg1>
class AbsUDF : public udf::ScalarUDF {
 public:
  TArg1 Exec(FunctionContext*, TArg1 v) { return std::abs(v.val); }


  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::InheritTypeFromArgs<AbsUDF>::Create({types::ST_BYTES, types::ST_THROUGHPUT_PER_NS,
                                                      types::ST_THROUGHPUT_BYTES_PER_NS,
                                                      types::ST_DURATION_NS, types::ST_PERCENT})};
  }
};

template <typename TArg1>
class SqrtUDF : public udf::ScalarUDF {
 public:
  Float64Value Exec(FunctionContext*, TArg1 v) { return std::sqrt(static_cast<double>(v.val)); }

};

template <typename TReturn, typename TArg1, typename TArg2>
class ModuloUDF : public udf::ScalarUDF {
 public:
  TReturn Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1.val % b2.val; }
};

template <typename TArg1, typename TArg2 = TArg1>
class LogicalOrUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1.val || b2.val; }
};

template <typename TArg1, typename TArg2 = TArg1>
class LogicalAndUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1.val && b2.val; }
};

template <typename TArg1>
class LogicalNotUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, TArg1 b1) { return !b1.val; }
};

template <typename TArg1>
class NegateUDF : public udf::ScalarUDF {
 public:
  TArg1 Exec(FunctionContext*, TArg1 b1) { return -b1.val; }
};

template <typename TArg1>
class InvertUDF : public udf::ScalarUDF {
 public:
  TArg1 Exec(FunctionContext*, TArg1 b1) { return ~b1.val; }

};

class CeilUDF : public udf::ScalarUDF {
 public:
  Int64Value Exec(FunctionContext*, Float64Value b1) {
    return static_cast<int64_t>(std::ceil(b1.val));
  }

};

class FloorUDF : public udf::ScalarUDF {
 public:
  Int64Value Exec(FunctionContext*, Float64Value b1) {
    return static_cast<int64_t>(std::floor(b1.val));
  }

};

template <typename TArg1, typename TArg2 = TArg1>
class EqualUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1 == b2; }
};

template <typename TArg1, typename TArg2 = TArg1>
class NotEqualUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1 != b2; }
};

class ApproxEqualUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, types::Float64Value b1, types::Float64Value b2) {
    return std::abs(b1.val - b2.val) < std::numeric_limits<double>::epsilon();
  }

};

class ApproxNotEqualUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, types::Float64Value b1, types::Float64Value b2) {
    return std::abs(b1.val - b2.val) > std::numeric_limits<double>::epsilon();
  }
};

template <typename TArg1, typename TArg2 = TArg1>
class GreaterThanUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1 > b2; }

};

template <typename TArg1, typename TArg2 = TArg1>
class GreaterThanEqualUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1 >= b2; }

};

template <typename TArg1, typename TArg2 = TArg1>
class LessThanUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1 < b2; }
};

template <typename TArg1, typename TArg2 = TArg1>
class LessThanEqualUDF : public udf::ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1 <= b2; }
};

udf::ScalarUDFDocBuilder BinDoc();

template <typename TReturn, typename TArg1 = TReturn, typename TArg2 = TReturn>
class BinUDF : public udf::ScalarUDF {
 public:
  TReturn Exec(FunctionContext*, TArg1 b1, TArg2 b2) { return b1.val - (b1.val % b2.val); }
};

// Special instantitization to handle float to int conversion
template <typename TReturn, typename TArg2>
class BinUDF<TReturn, types::Float64Value, TArg2> : public udf::ScalarUDF {
 public:
  TReturn Exec(FunctionContext*, Float64Value b1, Int64Value b2) {
    return static_cast<int64_t>(b1.val) - (static_cast<int64_t>(b1.val) % b2.val);
  }
};

class RoundUDF : public udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Float64Value value, Int64Value decimal_places) {
    return absl::StrFormat("%.*f", decimal_places.val, value.val);
  }
};

class TimeToInt64UDF : public udf::ScalarUDF {
 public:
  Int64Value Exec(FunctionContext*, Time64NSValue value) { return value.val; }
};

class Int64ToTimeUDF : public udf::ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext*, Int64Value value) { return value.val; }
};

template <typename TArg>
class MeanUDA : public udf::UDA {
 public:
  void Update(FunctionContext*, TArg arg) {
    info_.size++;
    info_.count += arg.val;
  }
  void Merge(FunctionContext*, const MeanUDA& other) {
    info_.size += other.info_.size;
    info_.count += other.info_.count;
  }
  Float64Value Finalize(FunctionContext*) { return info_.count / info_.size; }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::InheritTypeFromArgs<MeanUDA>::Create({types::ST_BYTES, types::ST_THROUGHPUT_PER_NS,
                                                       types::ST_THROUGHPUT_BYTES_PER_NS,
                                                       types::ST_DURATION_NS, types::ST_PERCENT})};
  }

  StringValue Serialize(FunctionContext*) {
    return StringValue(reinterpret_cast<char*>(&info_), sizeof(info_));
  }

  Status Deserialize(FunctionContext*, const StringValue& data) {
    std::memcpy(&info_, data.data(), sizeof(info_));
    return Status::OK();
  }

 protected:
  struct MeanInfo {
    uint64_t size = 0;
    double count = 0;
  };

  MeanInfo info_;
};

template <typename TArg, typename TAggType = TArg>
class SumUDA : public udf::UDA {
 public:
  void Update(FunctionContext*, TArg arg) { sum_ = sum_.val + arg.val; }
  void Merge(FunctionContext*, const SumUDA& other) { sum_ = sum_.val + other.sum_.val; }
  TAggType Finalize(FunctionContext*) { return sum_; }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::InheritTypeFromArgs<SumUDA>::Create({types::ST_BYTES, types::ST_THROUGHPUT_PER_NS,
                                                      types::ST_THROUGHPUT_BYTES_PER_NS,
                                                      types::ST_PERCENT})};
  }
  StringValue Serialize(FunctionContext*) { return sum_.Serialize(); }

  Status Deserialize(FunctionContext*, const StringValue& data) { return sum_.Deserialize(data); }


 protected:
  TAggType sum_ = 0;
};

template <typename TArg>
class MaxUDA : public udf::UDA {
 public:
  void Update(FunctionContext*, TArg arg) {
    if (max_.val < arg.val) {
      max_ = arg;
    }
  }
  void Merge(FunctionContext*, const MaxUDA& other) {
    if (other.max_.val > max_.val) {
      max_ = other.max_;
    }
  }
  TArg Finalize(FunctionContext*) { return max_; }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::InheritTypeFromArgs<MaxUDA>::Create({types::ST_BYTES, types::ST_THROUGHPUT_PER_NS,
                                                      types::ST_THROUGHPUT_BYTES_PER_NS,
                                                      types::ST_DURATION_NS, types::ST_PERCENT})};
  }


  StringValue Serialize(FunctionContext*) { return max_.Serialize(); }

  Status Deserialize(FunctionContext*, const StringValue& data) { return max_.Deserialize(data); }

 protected:
  TArg max_ = std::numeric_limits<typename types::ValueTypeTraits<TArg>::native_type>::min();
};

template <typename TArg>
class MinUDA : public udf::UDA {
 public:
  void Update(FunctionContext*, TArg arg) {
    if (min_.val > arg.val) {
      min_ = arg;
    }
  }
  void Merge(FunctionContext*, const MinUDA& other) {
    if (other.min_.val < min_.val) {
      min_ = other.min_;
    }
  }
  TArg Finalize(FunctionContext*) { return min_; }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::InheritTypeFromArgs<MinUDA>::Create({types::ST_BYTES, types::ST_THROUGHPUT_PER_NS,
                                                      types::ST_THROUGHPUT_BYTES_PER_NS,
                                                      types::ST_DURATION_NS, types::ST_PERCENT})};
  }

  StringValue Serialize(FunctionContext*) { return min_.Serialize(); }

  Status Deserialize(FunctionContext*, const StringValue& data) { return min_.Deserialize(data); }

 protected:
  TArg min_ = std::numeric_limits<typename types::ValueTypeTraits<TArg>::native_type>::max();
};

template <typename TArg>
class CountUDA : public udf::UDA {
 public:
  void Update(FunctionContext*, TArg) { count_.val++; }
  void Merge(FunctionContext*, const CountUDA& other) { count_.val += other.count_.val; }
  Int64Value Finalize(FunctionContext*) { return count_; }

  StringValue Serialize(FunctionContext*) { return count_.Serialize(); }

  Status Deserialize(FunctionContext*, const StringValue& data) { return count_.Deserialize(data); }


 protected:
  Int64Value count_ = 0;
};

void RegisterMathOpsOrDie(udf::Registry* registry);
}  // namespace builtins
}  // namespace carnot
}  // namespace px
