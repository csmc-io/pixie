#include "src/common/base/base.h"
#include "src/common/testing/testing.h"

#include "src/shared/prometheus/decoder.h"

namespace px {
namespace shared {
namespace prometheus {

TEST(DecoderTest, DecodeMetric) {
  std::string prom_text = R"(
container_log_messages_total{container_id="/k8s/default/otel-collector-cf5f55bcb-56wg8/otel-collector",level="unknown",machine_id="c605c971c9d0a43aa7618dd79e5b2500",pattern_hash="",sample="",source="stdout/stderr",system_uuid="c605c971-c9d0-a43a-a761-8dd79e5b2500"} 1.525271e+06
container_log_messages_total{container_id="/k8s/gmp-system/collector-6k786/prometheus",level="info",machine_id="c605c971c9d0a43aa7618dd79e5b2500",pattern_hash="",sample="",source="stdout/stderr",system_uuid="c605c971-c9d0-a43a-a761-8dd79e5b2500"} 3
container_log_messages_total{container_id="/k8s/kube-system/konnectivity-agent-88b55d7d8-v29ht/konnectivity-agent",level="info",machine_id="c605c971c9d0a43aa7618dd79e5b2500",pattern_hash="",sample="",source="stdout/stderr",system_uuid="c605c971-c9d0-a43a-a761-8dd79e5b2500"} 63
container_log_messages_total{container_id="/k8s/kube-system/konnectivity-agent-88b55d7d8-v29ht/konnectivity-agent-metrics-collector",level="error",machine_id="c605c971c9d0a43aa7618dd79e5b2500",pattern_hash="d41d8cd98f00b204e9800998ecf8427e",sample="{\"level\":\"error\",\"ts\":1720186811.939012,\"caller\":\"gcm/export.go:434\",\"msg\":\"Failed to export metrics to Cloud Monitoring\",\"error\":\"rpc error: code = PermissionDenied desc = Permission monitoring.timeSeries.create denied (or the resource may not exist).\",\"stacktrace\":\"google3/cloud/kubernetes/metrics/common/gcm/gcm.(*exporter).exportBuffer\\n\\tcloud/kubernetes/metrics/common/gcm/export.go:434\\ngoogle3/cloud/kubernetes/metrics/common/gcm/gcm.(*exporter).flush\\n\\tcloud/kubernetes/metrics/common/gcm/export.go:383\\ngoogle3/cloud/kubernetes/metrics/common/gcm/gcm.(*exporter).Flush\\n\\tcloud/kubernetes/metrics/common/gcm/export.go:365\\ngoogle3/cloud/kubernetes/metrics/components/collector/adapter/adapter.(*adapter).Finalize\\n\\tcloud/kubernetes/metrics/components/collector/adapter/consume.go:131\\ngoogle3/cloud/kubernetes/metrics/components/collector/prometheus/prometheus.(*parser).ParseText\\n\\tcloud/kubernetes/metrics/components/collector/prometheus/parse.go:158\\ngoogle3/cloud/kubernetes/metrics/components/collector/collector.runScrapeLoop\\n\\tcloud/kubernetes/metrics/components/collector/collector.go:103\\ngoogle3/cloud/kubernetes/metrics/components/collector/collector.Run\\n\\tcloud/kubernetes/metrics/components/collector/collector.go:81\\ngoogle3/cloud/kubernetes/metrics/components/collector/collector.Start.func1\\n\\tcloud/kubernetes/metrics/components/collector/multi_target_collector.go:45\"}",source="stdout/stderr",system_uuid="c605c971-c9d0-a43a-a761-8dd79e5b2500"} 18
container_log_messages_total{container_id="/k8s/kube-system/kube-dns-bd76ffc78-68wdb/kubedns-metrics-collector",level="error",machine_id="c605c971c9d0a43aa7618dd79e5b2500",pattern_hash="d41d8cd98f00b204e9800998ecf8427e",sample="{\"level\":\"error\",\"ts\":1720186648.5371022,\"caller\":\"gcm/export.go:498\",\"msg\":\"Failed to export self-observability metrics to Cloud Monitoring\",\"error\":\"rpc error: code = PermissionDenied desc = Permission monitoring.timeSeries.create denied (or the resource may not exist).\",\"stacktrace\":\"google3/cloud/kubernetes/metrics/common/gcm/gcm.(*exporter).startSelfObservability\\n\\tcloud/kubernetes/metrics/common/gcm/export.go:498\"}",source="stdout/stderr",system_uuid="c605c971-c9d0-a43a-a761-8dd79e5b2500"} 18
container_log_messages_total{container_id="/k8s/kube-system/kube-dns-bd76ffc78-68wdb/kubedns-metrics-collector",level="error",machine_id="c605c971c9d0a43aa7618dd79e5b2500",pattern_hash="d41d8cd98f00b204e9800998ecf8427e",sample="{\"level\":\"error\",\"ts\":1720186648.5371022,\"caller\":\"gcm/export.go:498\",\"msg\":\"Failed to export self-observability metrics to Cloud Monitoring\",\"error\":\"rpc error: code = PermissionDenied desc = Permission monitoring.timeSeries.create denied (or the resource may not exist).\",\"stacktrace\":\"google3/cloud/kubernetes/metrics/common/gcm/gcm.(*exporter).startSelfObservability\\n\\tcloud/kubernetes/metrics/common/gcm/export.go:498\"}",source="stdout/stderr",system_uuid="c605c971-c9d0-a43a-a761-8dd79e5b2500"} 2600
# HELP container_log_messages_total Number of messages grouped by the automatically extracted repeated patterntal Number of messages grouped by the automatically extracted repeated pattern)";

  std::string metric_name = "container_log_messages_total";
  std::vector<Metric> metrics = DecodeMetric(prom_text, metric_name);
  for (auto& metric : metrics) {
    EXPECT_EQ(metric.name, metric_name);
    if (metric.labels["container_id"] == "/k8s/default/otel-collector-cf5f55bcb-56wg8/otel-collector") {
      EXPECT_EQ(metric.val, 1525271);
    } else if (metric.labels["container_id"] == "/k8s/gmp-system/collector-6k786/prometheus") {
      EXPECT_EQ(metric.val, 3);
    } else if (metric.labels["container_id"] == "/k8s/kube-system/konnectivity-agent-88b55d7d8-v29ht/konnectivity-agent") {
      EXPECT_EQ(metric.val, 63);
    } else if (metric.labels["container_id"] == "/k8s/kube-system/konnectivity-agent-88b55d7d8-v29ht/konnectivity-agent-metrics-collector") {
      EXPECT_EQ(metric.val, 18);
    } else if (metric.labels["container_id"] == "/k8s/kube-system/kube-dns-bd76ffc78-68wdb/kubedns-metrics-collector") {
      EXPECT_EQ(metric.val, 18);
    } else if (metric.labels["container_id"] == "/k8s/kube-system/kube-dns-bd76ffc78-68wdb/kubedns-metrics-collector") {
      EXPECT_EQ(metric.val, 2600);
    }
  }

  LOG(INFO) << absl::StrCat("Metrics: ", ToString(metrics));
}

} // namespace prometheus
} // namespace shared
} // namespace px
