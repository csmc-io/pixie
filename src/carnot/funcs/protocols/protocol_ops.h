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

#include "src/carnot/udf/registry.h"
#include "src/shared/types/types.h"

namespace px {
namespace carnot {
namespace funcs {
namespace protocols {

class ProtocolNameUDF : public px::carnot::udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Int64Value resp_code);

};

class HTTPRespMessageUDF : public px::carnot::udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Int64Value resp_code);

};

class KafkaAPIKeyNameUDF : public px::carnot::udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Int64Value api_key);

};

class MySQLCommandNameUDF : public px::carnot::udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Int64Value api_key);

};

class CQLOpcodeNameUDF : public px::carnot::udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Int64Value req_op);

};

class AMQPFrameTypeUDF : public px::carnot::udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Int64Value frame_type);

};

class AMQPMethodTypeUDF : public px::carnot::udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Int64Value class_id, Int64Value method_id);

};

class MuxFrameTypeUDF : public px::carnot::udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Int64Value frame_type);

};

class DNSRcodeNameUDF : public px::carnot::udf::ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, Int64Value rcode);

};

void RegisterProtocolOpsOrDie(px::carnot::udf::Registry* registry);

}  // namespace protocols
}  // namespace funcs
}  // namespace carnot
}  // namespace px
