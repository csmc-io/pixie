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
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <absl/numeric/int128.h>
#include <grpcpp/grpcpp.h>
#include <magic_enum.hpp>

#include "src/carnot/udf/registry.h"
#include "src/carnot/udf/udf.h"
#include "src/common/base/base.h"
#include "src/common/uuid/uuid.h"
#include "src/shared/types/typespb/types.pb.h"
#include "src/vizier/services/agent/shared/manager/manager.h"
#include "src/vizier/services/metadata/metadatapb/service.grpc.pb.h"

namespace px {
namespace vizier {
namespace funcs {
namespace md {

constexpr std::string_view kKernelHeadersInstalledDesc =
    "Whether the agent had linux headers pre-installed";

template <typename TUDTF>
class UDTFWithMDFactory : public carnot::udf::UDTFFactory {
 public:
  UDTFWithMDFactory() = delete;
  explicit UDTFWithMDFactory(const VizierFuncFactoryContext& ctx) : ctx_(ctx) {}

  std::unique_ptr<carnot::udf::AnyUDTF> Make() override {
    return std::make_unique<TUDTF>(ctx_.mds_stub(), ctx_.add_auth_to_grpc_context_func());
  }

 private:
  const VizierFuncFactoryContext& ctx_;
};

template <typename TUDTF>
class UDTFWithCronscriptFactory : public carnot::udf::UDTFFactory {
 public:
  UDTFWithCronscriptFactory() = delete;
  explicit UDTFWithCronscriptFactory(const VizierFuncFactoryContext& ctx) : ctx_(ctx) {}

  std::unique_ptr<carnot::udf::AnyUDTF> Make() override {
    return std::make_unique<TUDTF>(ctx_.cronscript_stub(), ctx_.add_auth_to_grpc_context_func());
  }

 private:
  const VizierFuncFactoryContext& ctx_;
};

/**
 * This UDTF shows the status of each agent.
 */
class GetAgentStatus final : public carnot::udf::UDTF<GetAgentStatus> {
 public:
  using MDSStub = vizier::services::metadata::MetadataService::Stub;
  using SchemaResponse = vizier::services::metadata::SchemaResponse;
  GetAgentStatus() = delete;
  GetAgentStatus(std::shared_ptr<MDSStub> stub,
                 std::function<void(grpc::ClientContext*)> add_context_authentication)
      : idx_(0), stub_(stub), add_context_authentication_func_(add_context_authentication) {}

  static constexpr auto Executor() { return carnot::udfspb::UDTFSourceExecutor::UDTF_ONE_KELVIN; }

  static constexpr auto OutputRelation() {
    return MakeArray(
        ColInfo("agent_id", types::DataType::UINT128, types::PatternType::GENERAL,
                "The id of the agent"),
        ColInfo("asid", types::DataType::INT64, types::PatternType::GENERAL, "The Agent Short ID"),
        ColInfo("hostname", types::DataType::STRING, types::PatternType::GENERAL,
                "The hostname of the agent"),
        ColInfo("ip_address", types::DataType::STRING, types::PatternType::GENERAL,
                "The IP address of the agent"),
        ColInfo("agent_state", types::DataType::STRING, types::PatternType::GENERAL,
                "The current health status of the agent"),
        ColInfo("create_time", types::DataType::TIME64NS, types::PatternType::GENERAL,
                "The creation time of the agent"),
        ColInfo("last_heartbeat_ns", types::DataType::INT64, types::PatternType::GENERAL,
                "Time (in nanoseconds) since the last heartbeat"),
        ColInfo("kernel_headers_installed", types::DataType::BOOLEAN, types::PatternType::GENERAL,
                kKernelHeadersInstalledDesc));
  }

  Status Init(FunctionContext*, types::BoolValue include_kelvin) {
    px::vizier::services::metadata::AgentInfoRequest req;
    resp_ = std::make_unique<px::vizier::services::metadata::AgentInfoResponse>();
    include_kelvin_ = include_kelvin.val;

    grpc::ClientContext ctx;
    add_context_authentication_func_(&ctx);
    auto s = stub_->GetAgentInfo(&ctx, req, resp_.get());
    if (!s.ok()) {
      return error::Internal("Failed to make RPC call to GetAgentInfo");
    }
    return Status::OK();
  }

  static constexpr auto InitArgs() {
    return MakeArray(UDTFArg::Make<types::BOOLEAN>(
        "include_kelvin", "Whether to include Kelvin agents in the output", true));
  }

  bool NextRecord(FunctionContext*, RecordWriter* rw) {
    const auto& agent_metadata = resp_->info(idx_);
    const auto& agent_info = agent_metadata.agent();
    const auto& agent_status = agent_metadata.status();

    auto u_or_s = ParseUUID(agent_info.info().agent_id());
    sole::uuid u;
    if (u_or_s.ok()) {
      u = u_or_s.ConsumeValueOrDie();
    }
    // TODO(zasgar): Figure out abort mechanism;

    auto host_info = agent_info.info().host_info();
    auto collects_data = agent_info.info().capabilities().collects_data();
    if (collects_data || include_kelvin_) {
      rw->Append<IndexOf("agent_id")>(absl::MakeUint128(u.ab, u.cd));
      rw->Append<IndexOf("asid")>(agent_info.asid());
      rw->Append<IndexOf("hostname")>(host_info.hostname());
      rw->Append<IndexOf("ip_address")>(agent_info.info().ip_address());
      rw->Append<IndexOf("agent_state")>(StringValue(magic_enum::enum_name(agent_status.state())));
      rw->Append<IndexOf("create_time")>(agent_info.create_time_ns());
      rw->Append<IndexOf("last_heartbeat_ns")>(agent_status.ns_since_last_heartbeat());
      rw->Append<IndexOf("kernel_headers_installed")>(host_info.kernel_headers_installed());
    }

    ++idx_;
    return idx_ < resp_->info_size();
  }

 private:
  int idx_ = 0;
  bool include_kelvin_ = false;
  std::unique_ptr<px::vizier::services::metadata::AgentInfoResponse> resp_;
  std::shared_ptr<MDSStub> stub_;
  std::function<void(grpc::ClientContext*)> add_context_authentication_func_;
};


namespace internal {
inline rapidjson::GenericStringRef<char> StringRef(std::string_view s) {
  return rapidjson::GenericStringRef<char>(s.data(), s.size());
}

}  // namespace internal

class GetCronScriptHistory final : public carnot::udf::UDTF<GetCronScriptHistory> {
 public:
  using CronScriptStoreStub = vizier::services::metadata::CronScriptStoreService::Stub;
  GetCronScriptHistory() = delete;
  explicit GetCronScriptHistory(
      std::shared_ptr<CronScriptStoreStub> stub,
      std::function<void(grpc::ClientContext*)> add_context_authentication)
      : stub_(stub), add_context_authentication_func_(std::move(add_context_authentication)) {}

  static constexpr auto Executor() { return carnot::udfspb::UDTFSourceExecutor::UDTF_ONE_KELVIN; }

  static constexpr auto OutputRelation() {
    return MakeArray(
        ColInfo("script_id", types::DataType::STRING, types::PatternType::GENERAL,
                "The id of the cron script"),
        ColInfo("timestamp", types::DataType::TIME64NS, types::PatternType::GENERAL,
                "The time the script ran"),
        ColInfo("error_message", types::DataType::STRING, types::PatternType::GENERAL,
                "The error message if one exists"),
        ColInfo("execution_time_ns", types::DataType::INT64, types::PatternType::GENERAL,
                "The execution time of the script", types::SemanticType::ST_DURATION_NS),
        ColInfo("compilation_time_ns", types::DataType::INT64, types::PatternType::GENERAL,
                "The compiltation time of the script", types::SemanticType::ST_DURATION_NS),
        ColInfo("bytes_processed", types::DataType::INT64, types::PatternType::GENERAL,
                "The number of bytes processed during script execution",
                types::SemanticType::ST_BYTES),
        ColInfo("records_processed", types::DataType::INT64, types::PatternType::GENERAL,
                "The number of records processed during script execution"));
  }

  Status Init(FunctionContext*) {
    px::vizier::services::metadata::GetAllExecutionResultsRequest req;
    resp_ = std::make_unique<px::vizier::services::metadata::GetAllExecutionResultsResponse>();

    grpc::ClientContext ctx;
    add_context_authentication_func_(&ctx);
    auto s = stub_->GetAllExecutionResults(&ctx, req, resp_.get());
    if (!s.ok()) {
      return error::Internal("Failed to make RPC call to GetTracepointStatus: $0",
                             s.error_message());
    }
    return Status::OK();
  }

  bool NextRecord(FunctionContext*, RecordWriter* rw) {
    if (resp_->results_size() == 0) {
      return false;
    }
    const auto& result = resp_->results(idx_);
    auto u_or_s = ParseUUID(result.script_id());
    sole::uuid u;
    if (u_or_s.ok()) {
      u = u_or_s.ConsumeValueOrDie();
    }

    rw->Append<IndexOf("script_id")>(u.str());
    rw->Append<IndexOf("timestamp")>(types::Time64NSValue(
        result.timestamp().seconds() * 1000000000 + result.timestamp().nanos()));

    if (result.has_error()) {
      rw->Append<IndexOf("error_message")>(
          std::string(absl::Substitute("$0: $1 $2", result.error().err_code(), result.error().msg(),
                                       result.error().context().DebugString())));
      // set to 0.
      rw->Append<IndexOf("execution_time_ns")>(0);
      rw->Append<IndexOf("compilation_time_ns")>(0);
      rw->Append<IndexOf("bytes_processed")>(0);
      rw->Append<IndexOf("records_processed")>(0);
    } else {
      // Set to empty string.
      rw->Append<IndexOf("error_message")>("");
      const auto& exec_stats = result.execution_stats();
      rw->Append<IndexOf("execution_time_ns")>(exec_stats.execution_time_ns());
      rw->Append<IndexOf("compilation_time_ns")>(exec_stats.compilation_time_ns());
      rw->Append<IndexOf("bytes_processed")>(exec_stats.bytes_processed());
      rw->Append<IndexOf("records_processed")>(exec_stats.records_processed());
    }

    ++idx_;
    return idx_ < resp_->results_size();
  }

 private:
  int idx_ = 0;
  std::unique_ptr<px::vizier::services::metadata::GetAllExecutionResultsResponse> resp_;
  std::shared_ptr<CronScriptStoreStub> stub_;
  std::function<void(grpc::ClientContext*)> add_context_authentication_func_;
};

}  // namespace md
}  // namespace funcs
}  // namespace vizier
}  // namespace px
