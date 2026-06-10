#include "FrequentDataUtils.h"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <utility>

namespace UvUtils {

static const int64_t kMaxLogIntervalTimeMs = 10000;

LogAdapter FrequentDataLogger::s_log_adapter_ = nullptr;

void FrequentDataLogger::SetGlobalLogAdapter(LogAdapter log_adapter) {
  s_log_adapter_ = std::move(log_adapter);
}

void FrequentDataLogger::OnReport(FrequentDataLogLevel level,
                                  const std::string& /*tag*/,
                                  FrequentDataReportType /*type*/,
                                  const std::string& message) {
  if (s_log_adapter_) {
    s_log_adapter_(level, message);
  }
}

FrequentDataStatsCollector::ReportsByTag FrequentDataStatsCollector::TakeReports() {
  std::lock_guard<std::recursive_mutex> lock(data_loggers_mutex_);
  ReportsByTag result;
  result.swap(reports_);
  return result;
}

void FrequentDataStatsCollector::OnReport(FrequentDataLogLevel level,
                                          const std::string& tag,
                                          FrequentDataReportType type,
                                          const std::string& message) {
  std::lock_guard<std::recursive_mutex> lock(data_loggers_mutex_);
  TagReport& report = reports_[tag];
  switch (level) {
    case FrequentDataLogLevel::kWarning:
      report.warning_message = message;
      break;
    case FrequentDataLogLevel::kError:
      report.error_message = message;
      break;
    case FrequentDataLogLevel::kInfo:
    default:
      if (type == FrequentDataReportType::kBufferedData) {
        report.buffered_data_report = message;
      } else {
        report.interval_stats_report = message;
      }
      break;
  }
}

void FrequentDataAggregator::OnReport(FrequentDataLogLevel /*level*/,
                                      const std::string& /*tag*/,
                                      FrequentDataReportType /*type*/,
                                      const std::string& /*message*/) {
}

#define LOG_INTERVAL_DATA_BEGIN(VALUE_TYPE) \
  std::unique_lock<std::recursive_mutex> lock(data_loggers_mutex_); \
  if (data_loggers_.find(data_logger_tag) == data_loggers_.end()) { \
    AddOrUpdateDataLoggerInfo(data_logger_tag, now_ms, "", 1000); \
  } \
  auto ss = data_loggers_[data_logger_tag]->ss; \
  auto& interval_data_values = data_loggers_[data_logger_tag]->interval_data_values; \
  auto& last_data_for_interval = data_loggers_[data_logger_tag]->last_data_for_interval; \
  if (last_data_for_interval.value_type == IntervalData::kUndefined) { \
    last_data_for_interval.value_type = IntervalData::k##VALUE_TYPE; \
  } else if (last_data_for_interval.value_type != IntervalData::k##VALUE_TYPE) { \
    *ss << " <" << data << ">"; \
    TryFlush(now_ms); \
    return LogStream(ss, std::move(lock)); \
  } else {

#define LOG_INTERVAL_DATA_END(VALUE_TYPE) \
  } \
  last_data_for_interval.union_value.value_##VALUE_TYPE = data; \
  TryFlush(now_ms); \
  return LogStream(ss, std::move(lock));

FrequentDataAggregator::FrequentDataAggregator() {
}

FrequentDataAggregator::~FrequentDataAggregator() {
  std::lock_guard<std::recursive_mutex> lock(data_loggers_mutex_);
  TryFlush(std::numeric_limits<int64_t>::max());
  data_loggers_.clear();
  current_data_loggers_.clear();
}

void FrequentDataAggregator::AddOrUpdateDataLoggerInfo(
    const std::string& data_logger_tag,
    int64_t now_ms,
    const std::string& log_prefix,
    int64_t log_interval_time_ms) {
  std::lock_guard<std::recursive_mutex> lock(data_loggers_mutex_);

  bool is_new = (data_loggers_.find(data_logger_tag) == data_loggers_.end());
  if (is_new) {
    data_loggers_[data_logger_tag] = std::make_shared<DataLoggerInfo>();
  }

  if (log_interval_time_ms > kMaxLogIntervalTimeMs) {
    std::stringstream warn_ss;
    warn_ss << "data_logger_tag: " << data_logger_tag
            << " log_interval_time_ms=" << log_interval_time_ms
            << " is too large, set to " << kMaxLogIntervalTimeMs;
    OnReport(FrequentDataLogLevel::kWarning, data_logger_tag,
             FrequentDataReportType::kBufferedData, warn_ss.str());
    log_interval_time_ms = kMaxLogIntervalTimeMs;
  }

  data_loggers_[data_logger_tag]->tag = data_logger_tag;
  data_loggers_[data_logger_tag]->log_prefix = log_prefix;
  data_loggers_[data_logger_tag]->log_interval_time_ms = log_interval_time_ms;
  data_loggers_[data_logger_tag]->next_log_time_ms = now_ms + log_interval_time_ms;
  data_loggers_[data_logger_tag]->last_log_time_ms = now_ms;

  if (is_new) {
    current_data_loggers_.push_back(data_loggers_[data_logger_tag]);
  }
}

void FrequentDataAggregator::RemoveDataLoggerInfo(const std::string& data_logger_tag) {
  std::lock_guard<std::recursive_mutex> lock(data_loggers_mutex_);
  data_loggers_.erase(data_logger_tag);
}

LogStream FrequentDataAggregator::LogData(const std::string& data_logger_tag,
                                            int64_t now_ms) {
  std::unique_lock<std::recursive_mutex> lock(data_loggers_mutex_);
  if (data_loggers_.find(data_logger_tag) == data_loggers_.end()) {
    AddOrUpdateDataLoggerInfo(data_logger_tag, now_ms, "", 1000);
  }
  TryFlush(now_ms);
  return LogStream(data_loggers_[data_logger_tag]->ss, std::move(lock));
}

LogStream FrequentDataAggregator::LogIntervalData(const std::string& data_logger_tag,
                                                  int64_t now_ms,
                                                  double data) {
  LOG_INTERVAL_DATA_BEGIN(Double);
  double new_interval_data = data - last_data_for_interval.union_value.value_Double;
  interval_data_values.push_back(new_interval_data);
  LOG_INTERVAL_DATA_END(Double);
}

LogStream FrequentDataAggregator::LogIntervalData(const std::string& data_logger_tag,
                                                  int64_t now_ms,
                                                  int64_t data) {
  LOG_INTERVAL_DATA_BEGIN(Int64);
  int64_t new_interval_data = data - last_data_for_interval.union_value.value_Int64;
  interval_data_values.push_back(new_interval_data);
  LOG_INTERVAL_DATA_END(Int64);
}

LogStream FrequentDataAggregator::LogIntervalData(const std::string& data_logger_tag,
                                                  int64_t now_ms,
                                                  uint32_t data) {
  LOG_INTERVAL_DATA_BEGIN(Uint32);
  int64_t last_value = last_data_for_interval.unwrapped_value_for_unsigned_integer;
  int64_t new_interval_data =
      last_data_for_interval.UnwrapUnsignedInteger(data) - last_value;
  interval_data_values.push_back(new_interval_data);
  LOG_INTERVAL_DATA_END(Uint32);
}

LogStream FrequentDataAggregator::LogIntervalData(const std::string& data_logger_tag,
                                                  int64_t now_ms,
                                                  uint16_t data) {
  LOG_INTERVAL_DATA_BEGIN(Uint16);
  int64_t last_value = last_data_for_interval.unwrapped_value_for_unsigned_integer;
  int64_t new_interval_data =
      last_data_for_interval.UnwrapUnsignedInteger(data) - last_value;
  interval_data_values.push_back(new_interval_data);
  LOG_INTERVAL_DATA_END(Uint16);
}

LogStream FrequentDataAggregator::LogIntervalData(const std::string& data_logger_tag,
                                                  int64_t now_ms,
                                                  uint8_t data) {
  LOG_INTERVAL_DATA_BEGIN(Uint8);
  int64_t last_value = last_data_for_interval.unwrapped_value_for_unsigned_integer;
  int64_t new_interval_data =
      last_data_for_interval.UnwrapUnsignedInteger(data) - last_value;
  interval_data_values.push_back(new_interval_data);
  LOG_INTERVAL_DATA_END(Uint8);
}

LogStream FrequentDataAggregator::LogIntervalData(const std::string& data_logger_tag,
                                                  int64_t now_ms,
                                                  uint64_t data) {
  LOG_INTERVAL_DATA_BEGIN(Uint64);
  uint64_t last_value = last_data_for_interval.union_value.value_Uint64;
  uint64_t new_interval_data =
      last_data_for_interval.UnwrapUnsignedInteger(data) - last_value;
  interval_data_values.push_back(new_interval_data);
  LOG_INTERVAL_DATA_END(Uint64);
}

std::string FrequentDataAggregator::FormatBufferedDataReport(DataLoggerInfo& info) {
  if (info.ss->str().empty()) {
    return std::string();
  }

  std::stringstream log_ss;
  log_ss << info.tag;
  if (!info.log_prefix.empty()) {
    log_ss << "(" << info.log_prefix << ")";
  }
  log_ss << "=[ " << info.ss->str() << " ]";
  info.ss->str("");
  return log_ss.str();
}

std::string FrequentDataAggregator::FormatIntervalStatsReport(DataLoggerInfo& info) {
  if (info.interval_data_values.empty()) {
    return std::string();
  }

  double max_val = static_cast<double>(info.interval_data_values.front());
  double min_val = static_cast<double>(info.interval_data_values.front());
  double sum = 0;
  size_t cnt = 0;

  std::stringstream log_ss;
  std::stringstream interval_data_ss;
  log_ss << info.tag;
  if (!info.log_prefix.empty()) {
    log_ss << "(" << info.log_prefix << ")";
  }

  for (const auto& value : info.interval_data_values) {
    const double sample = static_cast<double>(value);
    if (sample > max_val) {
      max_val = sample;
    }
    if (sample < min_val) {
      min_val = sample;
    }
    sum += sample;
    ++cnt;
    interval_data_ss << " " << std::fixed << std::setprecision(3) << sample;
  }

  log_ss << std::fixed << std::setprecision(3)
         << " cps=" << (cnt * 1000.0 / info.log_interval_time_ms)
         << " avg=" << (sum / cnt)
         << " max=" << max_val
         << " min=" << min_val
         << ", [" << interval_data_ss.str() << " ]";
  info.interval_data_values.clear();
  return log_ss.str();
}

void FrequentDataAggregator::TryFlush(int64_t now_ms) {
  std::sort(current_data_loggers_.begin(), current_data_loggers_.end(),
            DataLoggerInfoComparator());
  auto it = current_data_loggers_.begin();
  while (it != current_data_loggers_.end()) {
    auto data_logger_info = it->lock();
    if (!data_logger_info) {
      it = current_data_loggers_.erase(it);
      continue;
    }
    if (now_ms >= data_logger_info->next_log_time_ms) {
      data_logger_info->next_log_time_ms =
          now_ms + data_logger_info->log_interval_time_ms;
      data_logger_info->last_log_time_ms = now_ms;

      const std::string buffered_report = FormatBufferedDataReport(*data_logger_info);
      if (!buffered_report.empty()) {
        OnReport(FrequentDataLogLevel::kInfo, data_logger_info->tag,
                 FrequentDataReportType::kBufferedData, buffered_report);
      }

      const std::string interval_report = FormatIntervalStatsReport(*data_logger_info);
      if (!interval_report.empty()) {
        OnReport(FrequentDataLogLevel::kInfo, data_logger_info->tag,
                 FrequentDataReportType::kIntervalStats, interval_report);
      }
    } else {
      break;
    }
    ++it;
  }
}

}  // namespace webrtc
