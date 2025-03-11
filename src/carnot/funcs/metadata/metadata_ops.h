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

#include <sys/sysinfo.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "src/carnot/funcs/shared/utils.h"
#include "src/carnot/udf/registry.h"
#include "src/carnot/udf/type_inference.h"
#include "src/shared/metadata/metadata_state.h"
#include "src/shared/types/types.h"

namespace px {
namespace carnot {
namespace funcs {
namespace metadata {

using ScalarUDF = px::carnot::udf::ScalarUDF;
using K8sNameIdentView = px::md::K8sMetadataState::K8sNameIdentView;

namespace internal {
inline rapidjson::GenericStringRef<char> StringRef(std::string_view s) {
  return rapidjson::GenericStringRef<char>(s.data(), s.size());
}

inline StatusOr<K8sNameIdentView> K8sName(std::string_view name) {
  std::vector<std::string_view> name_parts = absl::StrSplit(name, "/");
  if (name_parts.size() != 2) {
    return error::Internal("Malformed K8s name: $0", name);
  }
  return std::make_pair(name_parts[0], name_parts[1]);
}

}  // namespace internal

inline const px::md::AgentMetadataState* GetMetadataState(px::carnot::udf::FunctionContext* ctx) {
  DCHECK(ctx != nullptr);
  auto md = ctx->metadata_state();
  DCHECK(md != nullptr);
  return md;
}

class ASIDUDF : public ScalarUDF {
 public:
  Int64Value Exec(FunctionContext* ctx) {
    auto md = GetMetadataState(ctx);
    return md->asid();
  }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<ASIDUDF>(types::ST_ASID, {})};
  }
};

class UPIDToASIDUDF : public ScalarUDF {
 public:
  Int64Value Exec(FunctionContext*, UInt128Value upid_value) { return upid_value.High64() >> 32; }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<UPIDToASIDUDF>(types::ST_ASID, {types::ST_NONE})};
  }
};

class PodIDToPodNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info != nullptr) {
      return absl::Substitute("$0/$1", pod_info->ns(), pod_info->name());
    }

    return "";
  }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<PodIDToPodNameUDF>(types::ST_POD_NAME, {types::ST_NONE})};
  }
};

class PodIDToPodLabelsUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info != nullptr) {
      return pod_info->labels();
    }
    return "";
  }

};

class PodNameToPodIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);
    return GetPodID(md, pod_name);
  }

  static StringValue GetPodID(const px::md::AgentMetadataState* md, StringValue pod_name) {
    // This UDF expects the pod name to be in the format of "<ns>/<pod-name>".
    PX_ASSIGN_OR(auto pod_name_view, internal::K8sName(pod_name), return "");
    auto pod_id = md->k8s_metadata_state().PodIDByName(pod_name_view);
    return pod_id;
  }

};

class PodNameToPodIPUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);
    StringValue pod_id = PodNameToPodIDUDF::GetPodID(md, pod_name);
    const px::md::PodInfo* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }
    return pod_info->pod_ip();
  }

};

class PodIDToNamespaceUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info != nullptr) {
      return pod_info->ns();
    }

    return "";
  }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {
        udf::ExplicitRule::Create<PodIDToNamespaceUDF>(types::ST_NAMESPACE_NAME, {types::ST_NONE})};
  }
};

class PodNameToNamespaceUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue pod_name) {
    // This UDF expects the pod name to be in the format of "<ns>/<pod-name>".
    PX_ASSIGN_OR(auto k8s_name_view, internal::K8sName(pod_name), return "");
    return std::string(k8s_name_view.first);
  }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<PodNameToNamespaceUDF>(types::ST_NAMESPACE_NAME,
                                                             {types::ST_NONE})};
  }
};

class UPIDToContainerIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);

    auto upid_uint128 = absl::MakeUint128(upid_value.High64(), upid_value.Low64());
    auto upid = md::UPID(upid_uint128);
    auto pid = md->GetPIDByUPID(upid);
    if (pid == nullptr) {
      return "";
    }
    return pid->cid();
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

inline const md::ContainerInfo* UPIDToContainer(const px::md::AgentMetadataState* md,
                                                types::UInt128Value upid_value) {
  auto upid_uint128 = absl::MakeUint128(upid_value.High64(), upid_value.Low64());
  auto upid = md::UPID(upid_uint128);
  auto pid = md->GetPIDByUPID(upid);
  if (pid == nullptr) {
    return nullptr;
  }
  return md->k8s_metadata_state().ContainerInfoByID(pid->cid());
}

class UPIDToContainerNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto container_info = UPIDToContainer(md, upid_value);
    if (container_info == nullptr) {
      return "";
    }
    return std::string(container_info->name());
  }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<UPIDToContainerNameUDF>(types::ST_CONTAINER_NAME,
                                                              {types::ST_NONE})};
  }

  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

inline const px::md::PodInfo* UPIDtoPod(const px::md::AgentMetadataState* md,
                                        types::UInt128Value upid_value) {
  auto container_info = UPIDToContainer(md, upid_value);
  if (container_info == nullptr) {
    return nullptr;
  }
  auto pod_info = md->k8s_metadata_state().PodInfoByID(container_info->pod_id());
  return pod_info;
}

class UPIDToNamespaceUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);
    if (pod_info == nullptr) {
      return "";
    }
    return pod_info->ns();
  }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {
        udf::ExplicitRule::Create<UPIDToNamespaceUDF>(types::ST_NAMESPACE_NAME, {types::ST_NONE})};
  }

  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

class UPIDToPodIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto container_info = UPIDToContainer(md, upid_value);
    if (container_info == nullptr) {
      return "";
    }
    return std::string(container_info->pod_id());
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

class UPIDToPodNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);
    if (pod_info == nullptr) {
      return "";
    }
    return absl::Substitute("$0/$1", pod_info->ns(), pod_info->name());
  }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<UPIDToPodNameUDF>(types::ST_POD_NAME, {types::ST_NONE})};
  }

  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

class ServiceIDToServiceNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue service_id) {
    auto md = GetMetadataState(ctx);

    const auto* service_info = md->k8s_metadata_state().ServiceInfoByID(service_id);
    if (service_info != nullptr) {
      return absl::Substitute("$0/$1", service_info->ns(), service_info->name());
    }

    return "";
  }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<ServiceIDToServiceNameUDF>(types::ST_SERVICE_NAME,
                                                                 {types::ST_NONE})};
  }

};

class ServiceIDToClusterIPUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue service_id) {
    auto md = GetMetadataState(ctx);
    const auto* service_info = md->k8s_metadata_state().ServiceInfoByID(service_id);
    if (service_info != nullptr) {
      return service_info->cluster_ip();
    }
    return "";
  }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {
        udf::ExplicitRule::Create<ServiceIDToClusterIPUDF>(types::ST_IP_ADDRESS, {types::ST_NONE})};
  }
};

class ServiceIDToExternalIPsUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue service_id) {
    auto md = GetMetadataState(ctx);
    const auto* service_info = md->k8s_metadata_state().ServiceInfoByID(service_id);
    if (service_info != nullptr) {
      return VectorToStringArray(service_info->external_ips());
    }
    return "";
  }
};

class ServiceNameToServiceIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue service_name) {
    auto md = GetMetadataState(ctx);
    // This UDF expects the service name to be in the format of "<ns>/<service-name>".
    PX_ASSIGN_OR(auto service_name_view, internal::K8sName(service_name), return "");
    auto service_id = md->k8s_metadata_state().ServiceIDByName(service_name_view);
    return service_id;
  }
};

/**
 * @brief Returns the namespace for a service.
 */
class ServiceNameToNamespaceUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, StringValue service_name) {
    // This UDF expects the service name to be in the format of "<ns>/<svc-name>".
    PX_ASSIGN_OR(auto service_name_view, internal::K8sName(service_name), return "");
    return std::string(service_name_view.first);
  }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<ServiceNameToNamespaceUDF>(types::ST_NAMESPACE_NAME,
                                                                 {types::ST_NONE})};
  }
};

/**
 * @brief Returns the service ids for services that are currently running.
 */
class UPIDToServiceIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);
    if (pod_info == nullptr || pod_info->services().size() == 0) {
      return "";
    }
    std::vector<std::string> running_service_ids;
    for (const auto& service_id : pod_info->services()) {
      auto service_info = md->k8s_metadata_state().ServiceInfoByID(service_id);
      if (service_info == nullptr) {
        continue;
      }
      if (service_info->stop_time_ns() == 0) {
        running_service_ids.push_back(service_id);
      }
    }

    return StringifyVector(running_service_ids);
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

/**
 * @brief Returns the service names for services that are currently running.
 */
class UPIDToServiceNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);
    if (pod_info == nullptr || pod_info->services().size() == 0) {
      return "";
    }
    std::vector<std::string> running_service_names;
    for (const auto& service_id : pod_info->services()) {
      auto service_info = md->k8s_metadata_state().ServiceInfoByID(service_id);
      if (service_info == nullptr) {
        continue;
      }
      if (service_info->stop_time_ns() == 0) {
        running_service_names.push_back(
            absl::Substitute("$0/$1", service_info->ns(), service_info->name()));
      }
    }
    return StringifyVector(running_service_names);
  }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {
        udf::ExplicitRule::Create<UPIDToServiceNameUDF>(types::ST_SERVICE_NAME, {types::ST_NONE})};
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

/**
 * @brief Returns the node name for the pod associated with the input upid.
 */
class UPIDToNodeNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);
    if (pod_info == nullptr) {
      return "";
    }
    std::string foo = std::string(pod_info->node_name());
    return foo;
  }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<UPIDToNodeNameUDF>(types::ST_NODE_NAME, {types::ST_NONE})};
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

/**
 * @brief Returns string representation of condition status
 */
inline std::string conditionStatusToString(md::ConditionStatus status) {
  switch (status) {
    case md::ConditionStatus::kTrue:
      return "True";
    case md::ConditionStatus::kFalse:
      return "False";
    default:
      return "Unknown";
  }
}

/**
 * @brief Converts Replica Set info to a json string.
 */

inline types::StringValue ReplicaSetInfoToStatus(const px::md::ReplicaSetInfo* rs_info) {
  int replicas = 0;
  int fully_labeled_replicas = 0;
  int ready_replicas = 0;
  int available_replicas = 0;
  int observed_generation = 0;
  int requested_replicas = 0;
  std::string conditions = "";

  if (rs_info != nullptr) {
    replicas = rs_info->replicas();
    fully_labeled_replicas = rs_info->fully_labeled_replicas();
    ready_replicas = rs_info->ready_replicas();
    available_replicas = rs_info->available_replicas();
    observed_generation = rs_info->observed_generation();
    requested_replicas = rs_info->requested_replicas();

    auto rs_conditions = rs_info->conditions();
    for (const auto& condition : rs_info->conditions()) {
      std::string conditionStatus;
      conditions = absl::Substitute(R"("$0": "$1", $2)", condition.first,
                                    conditionStatusToString(condition.second), conditions);
    }
  }

  rapidjson::Document d;
  d.SetObject();
  d.AddMember("replicas", replicas, d.GetAllocator());
  d.AddMember("fully_labeled_replicas", fully_labeled_replicas, d.GetAllocator());
  d.AddMember("ready_replicas", ready_replicas, d.GetAllocator());
  d.AddMember("available_replicas", available_replicas, d.GetAllocator());
  d.AddMember("requested_replicas", requested_replicas, d.GetAllocator());
  d.AddMember("observed_generation", observed_generation, d.GetAllocator());
  d.AddMember("conditions", internal::StringRef(conditions), d.GetAllocator());
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  d.Accept(writer);
  return sb.GetString();
}

// Converts owner reference object into a json string
inline types::StringValue OwnerReferenceString(const px::md::OwnerReference& owner_reference) {
  std::string uid = owner_reference.uid;
  std::string kind = owner_reference.kind;
  std::string name = owner_reference.name;

  rapidjson::Document d;
  d.SetObject();
  d.AddMember("uid", internal::StringRef(uid), d.GetAllocator());
  d.AddMember("kind", internal::StringRef(kind), d.GetAllocator());
  d.AddMember("name", internal::StringRef(name), d.GetAllocator());
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  d.Accept(writer);
  return sb.GetString();
}

/**
 * @brief Returns the Replica Set name for the given Replica Set ID.
 * @brief Returns string representation of Deployment condition status
 */
inline std::string ConditionTypeToString(md::DeploymentConditionType status) {
  switch (status) {
    case md::DeploymentConditionType::kAvailable:
      return "Available";
    case md::DeploymentConditionType::kProgressing:
      return "Progressing";
    case md::DeploymentConditionType::kReplicaFailure:
      return "ReplicaFailure";
    default:
      return "Unknown";
  }
}

/**
 * @brief Converts Deployment info to a json string.
 */

inline types::StringValue DeploymentInfoToStatus(const px::md::DeploymentInfo* dep_info) {
  int replicas = 0;
  int updated_replicas = 0;
  int ready_replicas = 0;
  int available_replicas = 0;
  int unavailable_replicas = 0;
  int observed_generation = 0;
  int requested_replicas = 0;

  std::string conditions = "";

  if (dep_info != nullptr) {
    replicas = dep_info->replicas();
    updated_replicas = dep_info->updated_replicas();
    ready_replicas = dep_info->ready_replicas();
    available_replicas = dep_info->available_replicas();
    unavailable_replicas = dep_info->unavailable_replicas();
    observed_generation = dep_info->observed_generation();
    requested_replicas = dep_info->requested_replicas();

    auto rs_conditions = dep_info->conditions();
    for (const auto& condition : dep_info->conditions()) {
      std::string conditionStatus;
      conditions = absl::Substitute(R"("$0": "$1", $2)", ConditionTypeToString(condition.first),
                                    conditionStatusToString(condition.second), conditions);
    }
  }

  rapidjson::Document d;
  d.SetObject();
  d.AddMember("replicas", replicas, d.GetAllocator());
  d.AddMember("updated_replicas", updated_replicas, d.GetAllocator());
  d.AddMember("ready_replicas", ready_replicas, d.GetAllocator());
  d.AddMember("available_replicas", available_replicas, d.GetAllocator());
  d.AddMember("unavailable_replicas", unavailable_replicas, d.GetAllocator());
  d.AddMember("requested_replicas", requested_replicas, d.GetAllocator());
  d.AddMember("observed_generation", observed_generation, d.GetAllocator());
  d.AddMember("conditions", internal::StringRef(conditions), d.GetAllocator());
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  d.Accept(writer);
  return sb.GetString();
}

/**
 * @brief Returns the replica set id for the given replica set name.
 */
class ReplicaSetIDToReplicaSetNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_id) {
    auto md = GetMetadataState(ctx);
    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }

    return absl::Substitute("$0/$1", rs_info->ns(), rs_info->name());
  }

};

/**
 * @brief Returns the Replica Set start time for Replica Set ID.
 */
class ReplicaSetIDToStartTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue replica_set_id) {
    auto md = GetMetadataState(ctx);
    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return 0;
    }
    return rs_info->start_time_ns();
  }

};

/**
 * @brief Returns the Replica Set stop time for Replica Set ID.
 */
class ReplicaSetIDToStopTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue replica_set_id) {
    auto md = GetMetadataState(ctx);
    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return 0;
    }
    return rs_info->stop_time_ns();
  }

};

/**
 * @brief Returns the Replica Set namespace for Replica Set ID.
 */
class ReplicaSetIDToNamespaceUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_id) {
    auto md = GetMetadataState(ctx);
    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }
    return rs_info->ns();
  }

};

/**
 * @brief Returns the Replica Set owner references for Replica Sets ID.
 */
class ReplicaSetIDToOwnerReferencesUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_id) {
    auto md = GetMetadataState(ctx);
    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }

    std::vector<std::string> owner_references;
    for (const auto& owner_reference : rs_info->owner_references()) {
      owner_references.push_back(OwnerReferenceString(owner_reference));
    }

    return VectorToStringArray(owner_references);
  }

};

/**
 * @brief Returns the Replica Set status for Replica Set ID.
 */
class ReplicaSetIDToStatusUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_id) {
    auto md = GetMetadataState(ctx);
    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }

    return ReplicaSetInfoToStatus(rs_info);
  }

};

/**
 * @brief Returns the Deployment name for Replica Set ID.
 */
class ReplicaSetIDToDeploymentNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_id) {
    auto md = GetMetadataState(ctx);
    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }

    auto dep_info = md->k8s_metadata_state().OwnerDeploymentInfo(rs_info);
    if (dep_info == nullptr) {
      return "";
    }

    return absl::Substitute("$0/$1", dep_info->ns(), dep_info->name());
  }

};

/**
 * @brief Returns the Deployment ID for Replica Set ID.
 */
class ReplicaSetIDToDeploymentIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_id) {
    auto md = GetMetadataState(ctx);
    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }

    auto dep_info = md->k8s_metadata_state().OwnerDeploymentInfo(rs_info);
    if (dep_info == nullptr) {
      return "";
    }

    return dep_info->uid();
  }

};

/**
 * @brief Returns the Replica Set name from Replica Set ID.
 */
class ReplicaSetNameToReplicaSetIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Replica Set name to be in the format of "<ns>/<replica-set-name>".
    PX_ASSIGN_OR(auto rs_name_view, internal::K8sName(replica_set_name), return "");
    auto replica_set_id = md->k8s_metadata_state().ReplicaSetIDByName(rs_name_view);

    return replica_set_id;
  }

};

/**
 * @brief Returns the Replica Set start time for Replica Set name.
 */
class ReplicaSetNameToStartTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue replica_set_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Replica Set name to be in the format of "<ns>/<replica-set-name>".
    PX_ASSIGN_OR(auto rs_name_view, internal::K8sName(replica_set_name), return 0);
    auto replica_set_id = md->k8s_metadata_state().ReplicaSetIDByName(rs_name_view);

    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return 0;
    }
    return rs_info->start_time_ns();
  }

};

/**
 * @brief Returns the Replica Set stop time for Replica Set name.
 */
class ReplicaSetNameToStopTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue replica_set_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Replica Set name to be in the format of "<ns>/<replica-set-name>".
    PX_ASSIGN_OR(auto rs_name_view, internal::K8sName(replica_set_name), return 0);
    auto replica_set_id = md->k8s_metadata_state().ReplicaSetIDByName(rs_name_view);

    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return 0;
    }
    return rs_info->stop_time_ns();
  }

};

/**
 * @brief Returns the Replica Set namespace for Replica Set name.
 */
class ReplicaSetNameToNamespaceUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Replica Set name to be in the format of "<ns>/<replica-set-name>".
    PX_ASSIGN_OR(auto rs_name_view, internal::K8sName(replica_set_name), return "");
    auto replica_set_id = md->k8s_metadata_state().ReplicaSetIDByName(rs_name_view);

    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }
    return rs_info->ns();
  }

};

/**
 * @brief Returns the Replica Set owner references for Replica Set name.
 */
class ReplicaSetNameToOwnerReferencesUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Replica Set name to be in the format of "<ns>/<replica-set-name>".
    PX_ASSIGN_OR(auto rs_name_view, internal::K8sName(replica_set_name), return "");
    auto replica_set_id = md->k8s_metadata_state().ReplicaSetIDByName(rs_name_view);

    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }

    std::vector<std::string> owner_references;
    for (const auto& owner_reference : rs_info->owner_references()) {
      owner_references.push_back(OwnerReferenceString(owner_reference));
    }

    return VectorToStringArray(owner_references);
  }

};

/**
 * @brief Returns the Replica Set status for Replica Set name.
 */
class ReplicaSetNameToStatusUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Replica Set name to be in the format of "<ns>/<replica-set-name>".
    PX_ASSIGN_OR(auto rs_name_view, internal::K8sName(replica_set_name), return "");
    auto replica_set_id = md->k8s_metadata_state().ReplicaSetIDByName(rs_name_view);

    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }

    return ReplicaSetInfoToStatus(rs_info);
  }

};

/**
 * @brief Returns the Deployment name for Replica Set name.
 */
class ReplicaSetNameToDeploymentNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Replica Set name to be in the format of "<ns>/<replica-set-name>".
    PX_ASSIGN_OR(auto rs_name_view, internal::K8sName(replica_set_name), return "");
    auto replica_set_id = md->k8s_metadata_state().ReplicaSetIDByName(rs_name_view);

    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }

    auto dep_info = md->k8s_metadata_state().OwnerDeploymentInfo(rs_info);
    if (dep_info == nullptr) {
      return "";
    }

    return absl::Substitute("$0/$1", dep_info->ns(), dep_info->name());
  }

};

/**
 * @brief Returns the Deployment ID for Replica Set ID.
 */
class ReplicaSetNameToDeploymentIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue replica_set_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Replica Set name to be in the format of "<ns>/<replica-set-name>".
    PX_ASSIGN_OR(auto rs_name_view, internal::K8sName(replica_set_name), return "");
    auto replica_set_id = md->k8s_metadata_state().ReplicaSetIDByName(rs_name_view);

    auto rs_info = md->k8s_metadata_state().ReplicaSetInfoByID(replica_set_id);
    if (rs_info == nullptr) {
      return "";
    }

    auto dep_info = md->k8s_metadata_state().OwnerDeploymentInfo(rs_info);
    if (dep_info == nullptr) {
      return "";
    }

    return dep_info->uid();
  }

};

/**
 * @brief Returns the Deployment name for the given Deployment ID.
 */
class DeploymentIDToDeploymentNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue deployment_id) {
    auto md = GetMetadataState(ctx);
    auto dep_info = md->k8s_metadata_state().DeploymentInfoByID(deployment_id);
    if (dep_info == nullptr) {
      return "";
    }

    return absl::Substitute("$0/$1", dep_info->ns(), dep_info->name());
  }

};

/**
 * @brief Returns the Deployment start time for Deployment ID.
 */
class DeploymentIDToStartTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue deployment_id) {
    auto md = GetMetadataState(ctx);
    auto dep_info = md->k8s_metadata_state().DeploymentInfoByID(deployment_id);
    if (dep_info == nullptr) {
      return 0;
    }
    return dep_info->start_time_ns();
  }

};

/**
 * @brief Returns the Deployment stop time for Deployment ID.
 */
class DeploymentIDToStopTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue deployment_id) {
    auto md = GetMetadataState(ctx);
    auto dep_info = md->k8s_metadata_state().DeploymentInfoByID(deployment_id);
    if (dep_info == nullptr) {
      return 0;
    }
    return dep_info->stop_time_ns();
  }

};

/**
 * @brief Returns the Deployment namespace for Deployment ID.
 */
class DeploymentIDToNamespaceUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue deployment_id) {
    auto md = GetMetadataState(ctx);
    auto dep_info = md->k8s_metadata_state().DeploymentInfoByID(deployment_id);
    if (dep_info == nullptr) {
      return "";
    }
    return dep_info->ns();
  }

};

/**
 * @brief Returns the Deployment status for Deployment ID.
 */
class DeploymentIDToStatusUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue deployment_id) {
    auto md = GetMetadataState(ctx);
    auto dep_info = md->k8s_metadata_state().DeploymentInfoByID(deployment_id);
    if (dep_info == nullptr) {
      return "";
    }

    return DeploymentInfoToStatus(dep_info);
  }

};

/**
 * @brief Returns the Deployment ID from Deployment name.
 */
class DeploymentNameToDeploymentIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue deployment_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Deployment name to be in the format of "<ns>/<deployment-name>".
    PX_ASSIGN_OR(auto deployment_name_view, internal::K8sName(deployment_name), return "");
    auto deployment_id = md->k8s_metadata_state().DeploymentIDByName(deployment_name_view);

    return deployment_id;
  }

};

/**
 * @brief Returns the Deployment start time for Deployment name.
 */
class DeploymentNameToStartTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue deployment_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Deployment name to be in the format of "<ns>/<deployment-name>".
    PX_ASSIGN_OR(auto deployment_name_view, internal::K8sName(deployment_name), return 0);
    auto deployment_id = md->k8s_metadata_state().DeploymentIDByName(deployment_name_view);

    auto dep_info = md->k8s_metadata_state().DeploymentInfoByID(deployment_id);
    if (dep_info == nullptr) {
      return 0;
    }
    return dep_info->start_time_ns();
  }

};

/**
 * @brief Returns the Deployment stop time for Deployment name.
 */
class DeploymentNameToStopTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue deployment_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Deployment name to be in the format of "<ns>/<deployment-name>".
    PX_ASSIGN_OR(auto deployment_name_view, internal::K8sName(deployment_name), return 0);
    auto deployment_id = md->k8s_metadata_state().DeploymentIDByName(deployment_name_view);

    auto dep_info = md->k8s_metadata_state().DeploymentInfoByID(deployment_id);
    if (dep_info == nullptr) {
      return 0;
    }
    return dep_info->stop_time_ns();
  }

};

/**
 * @brief Returns the Deployment namespace for Deployment name.
 */
class DeploymentNameToNamespaceUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue deployment_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Deployment name to be in the format of "<ns>/<deployment-name>".
    PX_ASSIGN_OR(auto deployment_name_view, internal::K8sName(deployment_name), return "");
    auto deployment_id = md->k8s_metadata_state().DeploymentIDByName(deployment_name_view);

    auto dep_info = md->k8s_metadata_state().DeploymentInfoByID(deployment_id);
    if (dep_info == nullptr) {
      return "";
    }
    return dep_info->ns();
  }

};

/**
 * @brief Returns the Deployment status for Deployment name.
 */
class DeploymentNameToStatusUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue deployment_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the Deployment name to be in the format of "<ns>/<deployment-name>".
    PX_ASSIGN_OR(auto deployment_name_view, internal::K8sName(deployment_name), return "");
    auto deployment_id = md->k8s_metadata_state().DeploymentIDByName(deployment_name_view);

    auto dep_info = md->k8s_metadata_state().DeploymentInfoByID(deployment_id);
    if (dep_info == nullptr) {
      return "";
    }

    return DeploymentInfoToStatus(dep_info);
  }

};

/**
 * @brief Returns the Replica Set names for Replica Sets that are currently running.
 */
class UPIDToReplicaSetNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);

    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    if (rs_info->stop_time_ns() != 0) {
      return "";
    }
    return absl::Substitute("$0/$1", rs_info->ns(), rs_info->name());
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

/**
 * @brief Returns the Replica Set IDs for Replica Sets that are currently running.
 */
class UPIDToReplicaSetIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    if (rs_info->stop_time_ns() != 0) {
      return "";
    }
    return rs_info->uid();
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

/**
 * @brief Returns the Replica Set status for Replica Sets that are currently running.
 */
class UPIDToReplicaSetStatusUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    return ReplicaSetInfoToStatus(rs_info);
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

/**
 * @brief Returns the Deployment name for processes which are currently running.
 */
class UPIDToDeploymentNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);

    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    auto dep_info = md->k8s_metadata_state().OwnerDeploymentInfo(rs_info);
    if (dep_info == nullptr) {
      return "";
    }

    if (dep_info->stop_time_ns() != 0) {
      return "";
    }
    return absl::Substitute("$0/$1", dep_info->ns(), dep_info->name());
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

/**
 * @brief Returns the Deployment ID for process which is currently running.
 */
class UPIDToDeploymentIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    auto dep_info = md->k8s_metadata_state().OwnerDeploymentInfo(rs_info);
    if (dep_info == nullptr) {
      return "";
    }

    if (dep_info->stop_time_ns() != 0) {
      return "";
    }
    return dep_info->uid();
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

/**
 * @brief Returns the hostname for the pod associated with the input upid.
 */
class UPIDToHostnameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);
    if (pod_info == nullptr) {
      return "";
    }
    return pod_info->hostname();
  }

  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

/**
 * @brief Returns the service names for the given pod ID.
 */
class PodIDToServiceNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    std::vector<std::string> running_service_names;
    for (const auto& service_id : pod_info->services()) {
      auto service_info = md->k8s_metadata_state().ServiceInfoByID(service_id);
      if (service_info == nullptr) {
        continue;
      }
      if (service_info->stop_time_ns() == 0) {
        running_service_names.push_back(
            absl::Substitute("$0/$1", service_info->ns(), service_info->name()));
      }
    }
    return StringifyVector(running_service_names);
  }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {
        udf::ExplicitRule::Create<PodIDToServiceNameUDF>(types::ST_SERVICE_NAME, {types::ST_NONE})};
  }
};

/**
 * @brief Returns the service ids for the given pod ID.
 */
class PodIDToServiceIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    std::vector<std::string> running_service_ids;
    for (const auto& service_id : pod_info->services()) {
      auto service_info = md->k8s_metadata_state().ServiceInfoByID(service_id);
      if (service_info == nullptr) {
        continue;
      }
      if (service_info->stop_time_ns() == 0) {
        running_service_ids.push_back(service_id);
      }
    }
    return StringifyVector(running_service_ids);
  }
};

/**
 * @brief Returns the owner references for the given pod ID.
 */
class PodIDToOwnerReferencesUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    std::vector<std::string> owner_references;
    for (const auto& owner_reference : pod_info->owner_references()) {
      owner_references.push_back(OwnerReferenceString(owner_reference));
    }
    return VectorToStringArray(owner_references);
  }
};

/**
 * @brief Returns the owner references for the given pod name.
 */
class PodNameToOwnerReferencesUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);

    StringValue pod_id = PodNameToPodIDUDF::GetPodID(md, pod_name);
    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    std::vector<std::string> owner_references;
    for (const auto& owner_reference : pod_info->owner_references()) {
      owner_references.push_back(OwnerReferenceString(owner_reference));
    }
    return VectorToStringArray(owner_references);
  }
};

/**
 * @brief Returns the Node Name of a pod ID passed in.
 */
class PodIDToNodeNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }
    std::string foo = std::string(pod_info->node_name());
    return foo;
  }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<PodIDToNodeNameUDF>(types::ST_NODE_NAME, {types::ST_NONE})};
  }
};

/**
 * @brief Returns the ReplicaSet name of a pod ID passed in.
 */
class PodIDToReplicaSetNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    return absl::Substitute("$0/$1", rs_info->ns(), rs_info->name());
  }

};

/**
 * @brief Returns the ReplicaSet ID of a pod ID passed in.
 */
class PodIDToReplicaSetIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    return rs_info->uid();
  }

};

/**
 * @brief Returns the Deployment name of a pod ID passed in.
 */
class PodIDToDeploymentNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    auto dep_info = md->k8s_metadata_state().OwnerDeploymentInfo(rs_info);
    if (dep_info == nullptr) {
      return "";
    }

    return absl::Substitute("$0/$1", dep_info->ns(), dep_info->name());
  }

};

/**
 * @brief Returns the Deployment ID of a pod ID passed in.
 */
class PodIDToDeploymentIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    auto dep_info = md->k8s_metadata_state().OwnerDeploymentInfo(rs_info);
    if (dep_info == nullptr) {
      return "";
    }

    return dep_info->uid();
  }

};

/**
 * @brief Returns the ReplicaSet name of a pod name passed in.
 */
class PodNameToReplicaSetNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the pod name to be in the format of "<ns>/<pod-name>".
    PX_ASSIGN_OR(auto pod_name_view, internal::K8sName(pod_name), return "");
    auto pod_id = md->k8s_metadata_state().PodIDByName(pod_name_view);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    return absl::Substitute("$0/$1", rs_info->ns(), rs_info->name());
  }
};

/**
 * @brief Returns the ReplicaSet ID of a Pod name passed in.
 */
class PodNameToReplicaSetIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the pod name to be in the format of "<ns>/<pod-name>".
    PX_ASSIGN_OR(auto pod_name_view, internal::K8sName(pod_name), return "");
    auto pod_id = md->k8s_metadata_state().PodIDByName(pod_name_view);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    return rs_info->uid();
  }
};

/**
 * @brief Returns the Deployment name of a pod name passed in.
 */
class PodNameToDeploymentNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the pod name to be in the format of "<ns>/<pod-name>".
    PX_ASSIGN_OR(auto pod_name_view, internal::K8sName(pod_name), return "");
    auto pod_id = md->k8s_metadata_state().PodIDByName(pod_name_view);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    auto dep_info = md->k8s_metadata_state().OwnerDeploymentInfo(rs_info);
    if (dep_info == nullptr) {
      return "";
    }

    return absl::Substitute("$0/$1", dep_info->ns(), dep_info->name());
  }
};

/**
 * @brief Returns the Deployment ID of a Pod name passed in.
 */
class PodNameToDeploymentIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the pod name to be in the format of "<ns>/<pod-name>".
    PX_ASSIGN_OR(auto pod_name_view, internal::K8sName(pod_name), return "");
    auto pod_id = md->k8s_metadata_state().PodIDByName(pod_name_view);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    auto rs_info = md->k8s_metadata_state().OwnerReplicaSetInfo(pod_info);
    if (rs_info == nullptr) {
      return "";
    }

    auto dep_info = md->k8s_metadata_state().OwnerDeploymentInfo(rs_info);
    if (dep_info == nullptr) {
      return "";
    }

    return dep_info->uid();
  }
};

/**
 * @brief Returns the service names for the given pod name.
 */
class PodNameToServiceNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the pod name to be in the format of "<ns>/<pod-name>".
    PX_ASSIGN_OR(auto pod_name_view, internal::K8sName(pod_name), return "");
    auto pod_id = md->k8s_metadata_state().PodIDByName(pod_name_view);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    std::vector<std::string> running_service_names;
    for (const auto& service_id : pod_info->services()) {
      auto service_info = md->k8s_metadata_state().ServiceInfoByID(service_id);
      if (service_info == nullptr) {
        continue;
      }
      if (service_info->stop_time_ns() == 0) {
        running_service_names.push_back(
            absl::Substitute("$0/$1", service_info->ns(), service_info->name()));
      }
    }
    return StringifyVector(running_service_names);
  }
  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<PodNameToServiceNameUDF>(types::ST_SERVICE_NAME,
                                                               {types::ST_POD_NAME})};
  }
};

/**
 * @brief Returns the service ids for the given pod name.
 */
class PodNameToServiceIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);

    // This UDF expects the pod name to be in the format of "<ns>/<pod-name>".
    PX_ASSIGN_OR(auto pod_name_view, internal::K8sName(pod_name), return "");
    auto pod_id = md->k8s_metadata_state().PodIDByName(pod_name_view);

    const auto* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }

    std::vector<std::string> running_service_ids;
    for (const auto& service_id : pod_info->services()) {
      auto service_info = md->k8s_metadata_state().ServiceInfoByID(service_id);
      if (service_info == nullptr) {
        continue;
      }
      if (service_info->stop_time_ns() == 0) {
        running_service_ids.push_back(service_id);
      }
    }
    return StringifyVector(running_service_ids);
  }
};

class UPIDToStringUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext*, UInt128Value upid_value) {
    auto upid_uint128 = absl::MakeUint128(upid_value.High64(), upid_value.Low64());
    auto upid = md::UPID(upid_uint128);
    return upid.String();
  }
};

class UPIDToPIDUDF : public ScalarUDF {
 public:
  Int64Value Exec(FunctionContext*, UInt128Value upid_value) {
    auto upid_uint128 = absl::MakeUint128(upid_value.High64(), upid_value.Low64());
    auto upid = md::UPID(upid_uint128);
    return static_cast<int64_t>(upid.pid());
  }
};

class UPIDToStartTSUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext*, UInt128Value upid_value) {
    auto upid_uint128 = absl::MakeUint128(upid_value.High64(), upid_value.Low64());
    auto upid = md::UPID(upid_uint128);
    return static_cast<int64_t>(upid.start_ts());
  }
};

class PodIDToPodStartTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);
    const px::md::PodInfo* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return 0;
    }
    return pod_info->start_time_ns();
  }
};

class PodIDToPodStopTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue pod_id) {
    auto md = GetMetadataState(ctx);
    const px::md::PodInfo* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return 0;
    }
    return pod_info->stop_time_ns();
  }
};

class PodNameToPodStartTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);
    StringValue pod_id = PodNameToPodIDUDF::GetPodID(md, pod_name);
    const px::md::PodInfo* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return 0;
    }
    return pod_info->start_time_ns();
  }
};

class PodNameToPodStopTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);
    StringValue pod_id = PodNameToPodIDUDF::GetPodID(md, pod_name);
    const px::md::PodInfo* pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return 0;
    }
    return pod_info->stop_time_ns();
  }
};

class ContainerNameToContainerIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue container_name) {
    auto md = GetMetadataState(ctx);
    return md->k8s_metadata_state().ContainerIDByName(container_name);
  }

};

class ContainerIDToContainerStartTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue container_id) {
    auto md = GetMetadataState(ctx);
    const px::md::ContainerInfo* container_info =
        md->k8s_metadata_state().ContainerInfoByID(container_id);
    if (container_info == nullptr) {
      return 0;
    }
    return container_info->start_time_ns();
  }
};

class ContainerIDToContainerStopTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue container_id) {
    auto md = GetMetadataState(ctx);
    const px::md::ContainerInfo* container_info =
        md->k8s_metadata_state().ContainerInfoByID(container_id);
    if (container_info == nullptr) {
      return 0;
    }
    return container_info->stop_time_ns();
  }
};

class ContainerNameToContainerStartTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue container_name) {
    auto md = GetMetadataState(ctx);
    StringValue container_id = md->k8s_metadata_state().ContainerIDByName(container_name);
    const px::md::ContainerInfo* container_info =
        md->k8s_metadata_state().ContainerInfoByID(container_id);
    if (container_info == nullptr) {
      return 0;
    }
    return container_info->start_time_ns();
  }
};

class ContainerNameToContainerStopTimeUDF : public ScalarUDF {
 public:
  Time64NSValue Exec(FunctionContext* ctx, StringValue container_name) {
    auto md = GetMetadataState(ctx);
    StringValue container_id = md->k8s_metadata_state().ContainerIDByName(container_name);
    const px::md::ContainerInfo* container_info =
        md->k8s_metadata_state().ContainerInfoByID(container_id);
    if (container_info == nullptr) {
      return 0;
    }
    return container_info->stop_time_ns();
  }
};

inline std::string PodPhaseToString(const px::md::PodPhase& pod_phase) {
  switch (pod_phase) {
    case md::PodPhase::kRunning:
      return "Running";
    case md::PodPhase::kPending:
      return "Pending";
    case md::PodPhase::kSucceeded:
      return "Succeeded";
    case md::PodPhase::kFailed:
      return "Failed";
    case md::PodPhase::kTerminated:
      return "Terminated";
    case md::PodPhase::kUnknown:
    default:
      return "Unknown";
  }
}

inline types::StringValue PodInfoToPodStatus(const px::md::PodInfo* pod_info) {
  std::string phase = PodPhaseToString(md::PodPhase::kUnknown);
  std::string msg = "";
  std::string reason = "";
  bool ready_condition = false;

  if (pod_info != nullptr) {
    phase = PodPhaseToString(pod_info->phase());
    msg = pod_info->phase_message();
    reason = pod_info->phase_reason();

    auto pod_conditions = pod_info->conditions();
    auto ready_status = pod_conditions.find(md::PodConditionType::kReady);
    ready_condition = ready_status != pod_conditions.end() &&
                      (ready_status->second == md::ConditionStatus::kTrue);
  }

  rapidjson::Document d;
  d.SetObject();
  d.AddMember("phase", internal::StringRef(phase), d.GetAllocator());
  d.AddMember("message", internal::StringRef(msg), d.GetAllocator());
  d.AddMember("reason", internal::StringRef(reason), d.GetAllocator());
  d.AddMember("ready", ready_condition, d.GetAllocator());
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  d.Accept(writer);
  return sb.GetString();
}

class PodNameToPodStatusUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the Pod status for a passed in pod.
   *
   * @param ctx: the function context
   * @param pod_name: the Value containing a pod name.
   * @return StringValue: the status of the pod.
   */
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);
    StringValue pod_id = PodNameToPodIDUDF::GetPodID(md, pod_name);
    auto pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    return PodInfoToPodStatus(pod_info);
  }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {
        udf::ExplicitRule::Create<PodNameToPodStatusUDF>(types::ST_POD_STATUS, {types::ST_NONE})};
  }
};

class PodNameToPodReadyUDF : public ScalarUDF {
 public:
  BoolValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);
    StringValue pod_id = PodNameToPodIDUDF::GetPodID(md, pod_name);
    auto pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return false;
    }
    auto pod_conditions = pod_info->conditions();
    auto ready_status = pod_conditions.find(md::PodConditionType::kReady);
    if (ready_status == pod_conditions.end()) {
      return false;
    }
    return ready_status->second == md::ConditionStatus::kTrue;
  }

};

class PodNameToPodStatusMessageUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the Pod status message for a passed in pod.
   *
   * @param ctx: the function context
   * @param pod_name: the Value containing a pod name.
   * @return StringValue: the status message of the pod.
   */
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);
    StringValue pod_id = PodNameToPodIDUDF::GetPodID(md, pod_name);
    auto pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }
    return pod_info->phase_message();
  }
};

class PodNameToPodStatusReasonUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the Pod status reason for a passed in pod.
   *
   * @param ctx: the function context
   * @param pod_name: the Value containing a pod name.
   * @return StringValue: the status reason of the pod.
   */
  StringValue Exec(FunctionContext* ctx, StringValue pod_name) {
    auto md = GetMetadataState(ctx);
    StringValue pod_id = PodNameToPodIDUDF::GetPodID(md, pod_name);
    auto pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
    if (pod_info == nullptr) {
      return "";
    }
    return pod_info->phase_reason();
  }
};

inline std::string ContainerStateToString(const px::md::ContainerState& container_state) {
  switch (container_state) {
    case md::ContainerState::kRunning:
      return "Running";
    case md::ContainerState::kWaiting:
      return "Waiting";
    case md::ContainerState::kTerminated:
      return "Terminated";
    case md::ContainerState::kUnknown:
    default:
      return "Unknown";
  }
}

class ContainerIDToContainerStatusUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the Container status for a passed in container.
   *
   * @param ctx: the function context
   * @param container_id: the Value containing a container ID.
   * @return StringValue: the status of the container.
   */
  StringValue Exec(FunctionContext* ctx, StringValue container_id) {
    auto md = GetMetadataState(ctx);
    auto container_info = md->k8s_metadata_state().ContainerInfoByID(container_id);

    std::string state = ContainerStateToString(md::ContainerState::kUnknown);
    std::string msg = "";
    std::string reason = "";
    if (container_info != nullptr) {
      state = ContainerStateToString(container_info->state());
      msg = container_info->state_message();
      reason = container_info->state_reason();
    }
    rapidjson::Document d;
    d.SetObject();
    d.AddMember("state", internal::StringRef(state), d.GetAllocator());
    d.AddMember("message", internal::StringRef(msg), d.GetAllocator());
    d.AddMember("reason", internal::StringRef(reason), d.GetAllocator());
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    d.Accept(writer);
    return sb.GetString();
  }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<ContainerIDToContainerStatusUDF>(types::ST_CONTAINER_STATUS,
                                                                       {types::ST_NONE})};
  }

};

class UPIDToPodStatusUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the Pod status for a passed in UPID.
   *
   * @param ctx: the function context
   * @param upid_vlue: the UPID to query for.
   * @return StringValue: the status of the pod.
   */
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    return PodInfoToPodStatus(UPIDtoPod(md, upid_value));
  }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<UPIDToPodStatusUDF>(types::ST_POD_STATUS, {types::ST_NONE})};
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

class UPIDToCmdLineUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the cmdline for the upid.
   *
   * @param ctx: The function context.
   * @param upid_value: The UPID value
   * @return StringValue: the cmdline for the UPID.
   */
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    auto upid_uint128 = absl::MakeUint128(upid_value.High64(), upid_value.Low64());
    auto upid = md::UPID(upid_uint128);
    auto pid_info = md->GetPIDByUPID(upid);
    if (pid_info == nullptr) {
      return "";
    }
    return pid_info->cmdline();
  }


  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

inline std::string PodInfoToPodQoS(const px::md::PodInfo* pod_info) {
  if (pod_info == nullptr) {
    return "";
  }
  return std::string(magic_enum::enum_name(pod_info->qos_class()));
}

class UPIDToPodQoSUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the qos for the upid's pod.
   *
   * @param ctx: The function context.
   * @param upid_value: The UPID value
   * @return StringValue: the cmdline for the UPID.
   */
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value) {
    auto md = GetMetadataState(ctx);
    return PodInfoToPodQoS(UPIDtoPod(md, upid_value));
  }

  // This UDF can currently only run on PEMs, because only PEMs have the UPID information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }
};

class HostnameUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the hostname of the machine.
   */
  StringValue Exec(FunctionContext* ctx) {
    auto md = GetMetadataState(ctx);

    return md->hostname();
  }

};

class HostNumCPUsUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the number of CPUs on the machine.
   */
  Int64Value Exec(FunctionContext* /* ctx */) {
    const size_t ncpus = get_nprocs_conf();
    return ncpus;
  }

};

class IPToPodIDUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the pod id of pod with given pod_ip
   */
  StringValue Exec(FunctionContext* ctx, StringValue pod_ip) {
    auto md = GetMetadataState(ctx);
    return md->k8s_metadata_state().PodIDByIP(pod_ip);
  }

  // This UDF can currently only run on Kelvins, because only Kelvins have the IP to pod
  // information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_KELVIN; }
};

/**
 * This UDF is a compiler internal function. It should only be used on local IP addresses
 * since this function is forced to run on PEMs. In cases where the IP could be a remote address,
 * then it is more correct to have the function run on Kelvin (IPToPodIDUDF or IPToPodIDAtTimeUDF).
 */
class UPIDtoPodNameLocalAddrFallback : public ScalarUDF {
 public:
  /**
   * @brief Gets the pod name from UPID or from local addr if first lookup fails
   */
  StringValue Exec(FunctionContext* ctx, UInt128Value upid_value, StringValue pod_ip,
                   Time64NSValue time) {
    auto md = GetMetadataState(ctx);
    auto pod_info = UPIDtoPod(md, upid_value);
    if (pod_info == nullptr) {
      auto pod_id = md->k8s_metadata_state().PodIDByIPAtTime(pod_ip, time.val);
      pod_info = md->k8s_metadata_state().PodInfoByID(pod_id);
      if (pod_info == nullptr) {
        return "";
      }
    }
    return absl::Substitute("$0/$1", pod_info->ns(), pod_info->name());
  }

  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_PEM; }

  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<UPIDtoPodNameLocalAddrFallback>(
        types::ST_POD_NAME, {types::ST_NONE, types::ST_NONE, types::ST_NONE})};
  }
};

class IPToPodIDAtTimeUDF : public ScalarUDF {
 public:
  /**
   * @brief Gets the pod id of pod with given pod_ip
   */
  StringValue Exec(FunctionContext* ctx, StringValue pod_ip, Time64NSValue time) {
    auto md = GetMetadataState(ctx);
    return md->k8s_metadata_state().PodIDByIPAtTime(pod_ip, time.val);
  }

  // This UDF can currently only run on Kelvins, because only Kelvins have the IP to pod
  // information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_KELVIN; }
};

class IPToServiceIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue ip) {
    auto md = GetMetadataState(ctx);
    // First, check the list of Service Cluster IPs for this IP.
    auto service_id = md->k8s_metadata_state().ServiceIDByClusterIP(ip);
    if (service_id != "") {
      return service_id;
    }
    // Next, check to see if the IP is in the list of of Pod IPs.
    auto pod_id = md->k8s_metadata_state().PodIDByIP(ip);
    if (pod_id == "") {
      return "";
    }
    PodIDToServiceIDUDF udf;
    return udf.Exec(ctx, pod_id);
  }


  // This UDF can currently only run on Kelvins, because only Kelvins have the IP to pod
  // information.
  static udfspb::UDFSourceExecutor Executor() { return udfspb::UDFSourceExecutor::UDF_KELVIN; }
};

inline bool EqualsOrArrayContains(const std::string& input, const std::string& value) {
  rapidjson::Document doc;
  doc.Parse(input.c_str());
  if (!doc.IsArray()) {
    return input == value;
  }
  for (rapidjson::SizeType i = 0; i < doc.Size(); ++i) {
    if (!doc[i].IsString()) {
      return false;
    }
    auto str = doc[i].GetString();
    if (str == value) {
      return true;
    }
  }
  return false;
}

class HasServiceNameUDF : public ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, StringValue service, StringValue value) {
    return EqualsOrArrayContains(service, value);
  }

};

class HasServiceIDUDF : public ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, StringValue service, StringValue value) {
    return EqualsOrArrayContains(service, value);
  }

};

// Same functionality as HasServiceNameUDF but with a better name. New class to avoid breaking code
// which uses HasServiceNameUDF
class HasValueUDF : public ScalarUDF {
 public:
  BoolValue Exec(FunctionContext*, StringValue array_or_value, StringValue value) {
    return EqualsOrArrayContains(array_or_value, value);
  }

};

class VizierIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx) {
    auto md = GetMetadataState(ctx);
    return md->vizier_id().str();
  }

};

class VizierNameUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx) {
    auto md = GetMetadataState(ctx);
    return md->vizier_name();
  }

};

class VizierNamespaceUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx) {
    auto md = GetMetadataState(ctx);
    return md->vizier_namespace();
  }

};

class CreateUPIDUDF : public udf::ScalarUDF {
 public:
  UInt128Value Exec(FunctionContext* ctx, Int64Value pid, Int64Value pid_start_time) {
    auto md = GetMetadataState(ctx);
    auto upid = md::UPID(md->asid(), pid.val, pid_start_time.val);
    return upid.value();
  }


  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<CreateUPIDUDF>(types::ST_UPID, {})};
  }
};

class CreateUPIDWithASIDUDF : public udf::ScalarUDF {
 public:
  UInt128Value Exec(FunctionContext*, Int64Value asid, Int64Value pid, Int64Value pid_start_time) {
    auto upid = md::UPID(asid.val, pid.val, pid_start_time.val);
    return upid.value();
  }


  static udf::InfRuleVec SemanticInferenceRules() {
    return {udf::ExplicitRule::Create<CreateUPIDWithASIDUDF>(types::ST_UPID, {})};
  }
};

class GetClusterCIDRRangeUDF : public udf::ScalarUDF {
 public:
  Status Init(FunctionContext* ctx) {
    auto md = GetMetadataState(ctx);
    std::set<std::string> cidr_strs;

    auto pod_cidrs = md->k8s_metadata_state().pod_cidrs();
    for (const auto& cidr : pod_cidrs) {
      cidr_strs.insert(cidr.ToString());

      if (cidr.ip_addr.family == px::InetAddrFamily::kIPv4) {
        // Add a CIDR specifically for the CNI bridge, since pod_cidrs don't include that.
        px::CIDRBlock bridge_cidr;
        bridge_cidr.ip_addr = cidr.ip_addr;
        bridge_cidr.prefix_length = 32;
        auto& ipv4_addr = std::get<struct in_addr>(bridge_cidr.ip_addr.addr);
        // Replace the lowest byte with 1, since s_addr is in big endian, we replace the first
        // byte.
        ipv4_addr.s_addr = (ipv4_addr.s_addr & 0x00ffffff) | 0x01000000;

        cidr_strs.insert(bridge_cidr.ToString());
      }
      // TODO(james): add support for IPv6 CNI bridge.
    }
    auto service_cidr = md->k8s_metadata_state().service_cidr();
    if (service_cidr.has_value()) {
      cidr_strs.insert(service_cidr.value().ToString());
    }
    std::vector<std::string> cidr_vec(cidr_strs.begin(), cidr_strs.end());
    cidrs_str_ = VectorToStringArray(cidr_vec);
    return Status::OK();
  }

  StringValue Exec(FunctionContext*) { return cidrs_str_; }


 private:
  std::string cidrs_str_;
};

class NamespaceNameToNamespaceIDUDF : public ScalarUDF {
 public:
  StringValue Exec(FunctionContext* ctx, StringValue namespace_name) {
    auto md = GetMetadataState(ctx);
    auto namespace_id =
        md->k8s_metadata_state().NamespaceIDByName(std::make_pair(namespace_name, namespace_name));
    return namespace_id;
  }

};

void RegisterMetadataOpsOrDie(px::carnot::udf::Registry* registry);

}  // namespace metadata
}  // namespace funcs
}  // namespace carnot
}  // namespace px
