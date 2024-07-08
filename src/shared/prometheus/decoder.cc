#include "src/shared/prometheus/decoder.h"

#include <cpr/cpr.h>

namespace px {
namespace shared {
namespace prometheus {

std::string FetchPrometheusMetrics(const std::string& url) {
    cpr::Response r = cpr::Get(cpr::Url{url});
    if(r.status_code == 0) {
        LOG(ERROR) << absl::Substitute("Failed to fetch prometheus metrics from $0 error=$1", url, r.error.message);
    } else if (r.status_code >= 400) {
        LOG(ERROR) << absl::Substitute("Failed to fetch prometheus metrics from $0 status_code=$1", url, r.status_code);
    }
    return r.text;
}

std::vector<Metric> DecodeMetric(const std::string& prom_text, const std::string& metric_name) {
    // parse prometheus text output for metrics that are named metric_name
    std::vector<Metric> metrics;
    std::string line;
    std::istringstream prom_text_stream(prom_text);
    while (std::getline(prom_text_stream, line)) {
        auto first_non_whitespace = line.find_first_not_of(" \t");
        if (first_non_whitespace != std::string::npos) {
            if (line.substr(first_non_whitespace, 1) == "#") {
                continue;
            }
        }
        if (line.find(metric_name) != std::string::npos) {
            Metric metric;
            metric.name = metric_name;
            std::string label_str = line.substr(line.find('{') + 1, line.find_last_of('}') - line.find('{') - 1);
            std::istringstream label_stream(label_str);
            std::string label;

            while (std::getline(label_stream, label, ',')) {
                if (label_stream.peek() == '\\') {
                    std::string cont;
                    while (std::getline(label_stream, cont, ',')) {
                        label += cont;
                    }
                }
                std::string key = label.substr(0, label.find('='));
                std::string val = label.substr(label.find('=') + 1);
                metric.labels[key] = val;
            }
            std::string val_str = line.substr(line.find_last_of('}') + 1);
            LOG(WARNING) << absl::Substitute("val_str=$0", val_str);
            metric.val = std::stoull(val_str);
            metrics.push_back(metric);
        }
    }
    return metrics;
};

}  // namespace prometheus
}  // namespace shared
}  // namespace px
