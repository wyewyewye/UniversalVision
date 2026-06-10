#ifndef FREQUENT_DATA_LOGGER_H_
#define FREQUENT_DATA_LOGGER_H_

#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace UvUtils {

// Log level for the log adapter
enum class FrequentDataLogLevel {
  kInfo,
  kWarning,
  kError
};

// Log adapter function type.
// Users can provide their own logging implementation to avoid dependency on specific log modules.
// Example:
//   auto adapter = [](FrequentDataLogLevel level, const std::string& msg) {
//     std::cout << "[" << static_cast<int>(level) << "] " << msg << std::endl;
//   };
//   FrequentDataLogger logger;
//   FrequentDataLogger::SetGlobalLogAdapter(adapter);
using LogAdapter = std::function<void(FrequentDataLogLevel level, const std::string& message)>;

enum class FrequentDataReportType {
  kBufferedData,
  kIntervalStats,
};

class FrequentDataAggregator;

// LogStream: RAII wrapper that holds the mutex lock while allowing << operations.
// The lock is automatically released when this object is destroyed (end of statement).
//
// Usage:
//   logger.LogData("tag", now_ms) << value1 << " " << value2;
//   // Lock is released here when temporary LogStream is destroyed
//
// Thread safety:
//   - The lock is held for the entire lifetime of this object.
//   - The stringstream is held via shared_ptr, ensuring it remains valid even if
//     the FrequentDataAggregator is destroyed or the tag is removed during use.
class LogStream {
 public:
  LogStream(std::shared_ptr<std::stringstream> ss, std::unique_lock<std::recursive_mutex> lock)
      : ss_(std::move(ss)), lock_(std::move(lock)) {}

  // Move constructor (for returning from functions)
  LogStream(LogStream&& other) noexcept
      : ss_(std::move(other.ss_)), lock_(std::move(other.lock_)) {}

  // Disable copy
  LogStream(const LogStream&) = delete;
  LogStream& operator=(const LogStream&) = delete;
  LogStream& operator=(LogStream&&) = delete;

  // Chain << operators, returns reference to self for continued chaining
  template<typename T>
  LogStream& operator<<(const T& value) {
    if (ss_) {
      *ss_ << value;
    }
    return *this;
  }

  // Support for stream manipulators (e.g., std::endl, std::hex)
  LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
    if (ss_) {
      manip(*ss_);
    }
    return *this;
  }

  // Lock is automatically released when LogStream is destroyed
  ~LogStream() = default;

 private:
  std::shared_ptr<std::stringstream> ss_;  // Shared ownership ensures validity
  std::unique_lock<std::recursive_mutex> lock_;
};

// FrequentDataAggregator: Core utility for throttled aggregation of high-frequency data.
//
// This class helps aggregate data that would otherwise flood logs or reports if printed
// on every occurrence. It maintains internal stringstreams for each tag and flushes
// them at configurable intervals. Formatted report strings are delivered via OnReport().
//
// Usage patterns:
//
// 1. Basic usage - accumulate data and flush periodically:
//    ```cpp
//    // Data is appended to internal buffer, flushed every 1000ms (default)
//    aggregator.LogData("packet_seq", now_ms) << seq_num << " ";
//    ```
//    Output: `packet_seq=[ 100 101 102 103 ]`
//
// 2. With custom prefix and interval:
//    ```cpp
//    aggregator.AddOrUpdateDataLoggerInfo("bitrate", now_ms, "video", 2000);
//    aggregator.LogData("bitrate", now_ms) << current_bitrate;
//    ```
//    Output: `bitrate(video)=[ 1500000 ]`
//
// 3. Interval data - track incremental changes (useful for counters):
//    ```cpp
//    // Records the difference since last call, handles wraparound for unsigned types
//    aggregator.LogIntervalData("bytes_sent", now_ms, total_bytes);
//    ```
//    Output: `bytes_sent cps=... avg=... max=... min=..., [ 1024 2048 512 ]`
//
// Thread safety:
//   - All public methods are protected by a recursive mutex.
//   - LogData/LogIntervalData return a LogStream RAII object that holds the lock
//     until the object is destroyed (typically at end of statement), ensuring
//     thread-safe access to the underlying stringstream during << operations.
//
class FrequentDataAggregator {
 public:
  FrequentDataAggregator();
  virtual ~FrequentDataAggregator();

  // Register or update a data logger with custom settings.
  //
  // @param data_logger_tag   Unique identifier for this logger (e.g., "packet_loss")
  // @param now_ms            Current timestamp in milliseconds
  // @param log_prefix        Optional prefix shown in parentheses (e.g., "video" -> "tag(video)=...")
  // @param log_interval_time_ms  Interval between flush outputs (capped at 10000ms)
  //
  // Note: If called multiple times for the same tag, updates the settings.
  //       The tag is auto-created with default 1000ms interval if LogData is called first.
  void AddOrUpdateDataLoggerInfo(const std::string& data_logger_tag,
                                 int64_t now_ms,
                                 const std::string& log_prefix,
                                 int64_t log_interval_time_ms);

  // Remove a data logger by tag. Pending data will not be flushed.
  //
  // @param data_logger_tag  The tag to remove
  void RemoveDataLoggerInfo(const std::string& data_logger_tag);

  // Append arbitrary data to the logger's buffer and trigger periodic flush.
  //
  // @param data_logger_tag  Logger identifier (auto-created if not exists, default 1000ms interval)
  // @param now_ms           Current timestamp in milliseconds
  // @return LogStream RAII object that holds the lock and supports << operations
  //
  // Example:
  //   aggregator.LogData("recovered_seq", now_ms) << seq << "=(" << type << ") ";
  //   // Lock is automatically released at end of statement
  //
  // The buffer is flushed when log_interval_time_ms has elapsed.
  // Output format: `tag(prefix)=[ buffered_data ]`
  LogStream LogData(const std::string& data_logger_tag, int64_t now_ms);

  // Log incremental/delta data for cumulative counters.
  //
  // These methods track the last value and output the DIFFERENCE since the previous call.
  // For unsigned integer types, automatic wraparound handling is provided.
  //
  // @param data_logger_tag  Logger identifier (auto-created if not exists)
  // @param now_ms           Current timestamp in milliseconds
  // @param data             Current cumulative value (delta is computed internally)
  // @return LogStream RAII object that holds the lock and supports << operations
  //
  // Example:
  //   // If total_bytes goes: 100 -> 200 -> 350
  //   // Output will show deltas: " 100 150"
  //   aggregator.LogIntervalData("bytes_received", now_ms, total_bytes);
  //
  // Note: First call establishes baseline (outputs nothing or 0).
  //       Type must remain consistent across calls for the same tag.
  LogStream LogIntervalData(const std::string& data_logger_tag,
                            int64_t now_ms,
                            double data);
  LogStream LogIntervalData(const std::string& data_logger_tag,
                            int64_t now_ms,
                            int64_t data);
  LogStream LogIntervalData(const std::string& data_logger_tag,
                            int64_t now_ms,
                            uint32_t data);
  LogStream LogIntervalData(const std::string& data_logger_tag,
                            int64_t now_ms,
                            uint16_t data);
  LogStream LogIntervalData(const std::string& data_logger_tag,
                            int64_t now_ms,
                            uint8_t data);
  LogStream LogIntervalData(const std::string& data_logger_tag,
                            int64_t now_ms,
                            uint64_t data);

 protected:
  // IntervalData: Tracks the last value for computing deltas in LogIntervalData.
  // Handles unsigned integer wraparound by unwrapping values to int64_t.
  struct IntervalData {
    IntervalData()
        : value_type(kUndefined)
        , unwrapper_round(0)
        , unwrapped_value_for_unsigned_integer(-1) {
      memset(&union_value, 0, sizeof(union_value));
    }

    // Type discriminator for the union
    enum {
      kUndefined,
      kDouble,
      kInt64,
      kUint64,
      kUint32,
      kUint16,
      kUint8,
    } value_type;

    // Storage for the last raw value
    union {
      double value_Double;
      int64_t value_Int64;
      uint64_t value_Uint64;
      uint32_t value_Uint32;
      uint16_t value_Uint16;
      uint8_t value_Uint8;
    } union_value;

    // Unwrap unsigned integer to handle wraparound.
    // E.g., for uint16_t: 65530 -> 65535 -> 0 -> 5 becomes 65530 -> 65535 -> 65536 -> 65541
    // Returns the unwrapped value as int64_t for safe delta computation.
    template<typename T, typename std::enable_if<std::is_unsigned<T>::value>::type* dummy = nullptr>
    int64_t UnwrapUnsignedInteger(T new_value) {
      if (unwrapped_value_for_unsigned_integer == -1) {
        SetUnionValue(new_value);
        unwrapped_value_for_unsigned_integer = new_value;
        return unwrapped_value_for_unsigned_integer;
      }

      T last_value = GetUnionValue<T>();
      T breakpoint = std::numeric_limits<T>::max() / 2 + T(1);
      // Detect forward wraparound: new_value < last_value but they are close (crossed max)
      if (new_value < last_value) {
        // new_value - last_value wraps to a large positive value if forward wrap occurred
        if (static_cast<T>(new_value - last_value) < breakpoint) {
          ++unwrapper_round;
        }
      // Detect backward movement (rare): large decrease suggests backward wrap
      } else if (static_cast<T>(new_value - last_value) > breakpoint) {
        --unwrapper_round;
      }

      SetUnionValue(new_value);
      unwrapped_value_for_unsigned_integer = new_value +
          unwrapper_round * (static_cast<int64_t>(std::numeric_limits<T>::max()) + 1);
      return unwrapped_value_for_unsigned_integer;
    }

    // Specialization for uint64_t: no wraparound handling (would overflow int64_t)
    uint64_t UnwrapUnsignedInteger(uint64_t new_value) {
      memcpy(&unwrapped_value_for_unsigned_integer, &new_value, sizeof(int64_t));
      SetUnionValue(new_value);
      return static_cast<uint64_t>(unwrapped_value_for_unsigned_integer);
    }

    template<typename T, typename std::enable_if<std::is_unsigned<T>::value>::type* dummy = nullptr>
    T GetUnionValue() {
      if (std::is_same<T, uint64_t>::value) {
        return union_value.value_Uint64;
      } else if (std::is_same<T, uint32_t>::value) {
        return union_value.value_Uint32;
      } else if (std::is_same<T, uint16_t>::value) {
        return union_value.value_Uint16;
      } else if (std::is_same<T, uint8_t>::value) {
        return union_value.value_Uint8;
      }
      return 0;
    }

    template<typename T, typename std::enable_if<std::is_unsigned<T>::value>::type* dummy = nullptr>
    void SetUnionValue(T new_value) {
      if (std::is_same<T, uint64_t>::value) {
        union_value.value_Uint64 = new_value;
      } else if (std::is_same<T, uint32_t>::value) {
        union_value.value_Uint32 = new_value;
      } else if (std::is_same<T, uint16_t>::value) {
        union_value.value_Uint16 = new_value;
      } else if (std::is_same<T, uint8_t>::value) {
        union_value.value_Uint8 = new_value;
      }
    }

    int64_t unwrapper_round;                      // Number of times the value has wrapped around
    int64_t unwrapped_value_for_unsigned_integer; // Last unwrapped value (-1 = uninitialized)
  };

  // Per-tag logging state
  struct DataLoggerInfo {
    DataLoggerInfo()
        : ss(std::make_shared<std::stringstream>())
        , log_interval_time_ms(1000)
        , next_log_time_ms(0)
        , last_log_time_ms(0) {}

    std::string tag;                              // Logger identifier
    std::string log_prefix;                       // Optional prefix for output
    std::shared_ptr<std::stringstream> ss;        // Buffer for accumulated data (shared for LogStream safety)
    std::vector<int64_t> interval_data_values;    // Values for interval data
    int64_t log_interval_time_ms;                 // Flush interval
    int64_t next_log_time_ms;                     // Next scheduled flush time
    int64_t last_log_time_ms;                     // Last flush time

    IntervalData last_data_for_interval;          // State for LogIntervalData
  };

  // Comparator for sorting loggers by next flush time
  class DataLoggerInfoComparator {
   public:
    bool operator()(const std::weak_ptr<DataLoggerInfo>& a,
                    const std::weak_ptr<DataLoggerInfo>& b) const {
      auto a_ptr = a.lock();
      auto b_ptr = b.lock();
      if (!a_ptr && b_ptr) {
        return true;
      }
      if (a_ptr && !b_ptr) {
        return false;
      }
      if (!a_ptr && !b_ptr) {
        return true;
      }
      return a_ptr->next_log_time_ms < b_ptr->next_log_time_ms;
    }
  };

  // Called when a formatted report is ready. Override to customize output behavior.
  // @param tag   data_logger_tag for the report source
  // @param type  report category (buffered data vs interval statistics)
  virtual void OnReport(FrequentDataLogLevel level,
                        const std::string& tag,
                        FrequentDataReportType type,
                        const std::string& message);

  // Check all loggers and flush those whose interval has elapsed.
  // Sorts loggers by next_log_time_ms and delivers ready reports via OnReport().
  void TryFlush(int64_t now_ms);

  static std::string FormatBufferedDataReport(DataLoggerInfo& info);
  static std::string FormatIntervalStatsReport(DataLoggerInfo& info);

  std::recursive_mutex data_loggers_mutex_;       // Protects all member access
  std::unordered_map<std::string, std::shared_ptr<DataLoggerInfo>> data_loggers_;  // tag -> logger
  std::vector<std::weak_ptr<DataLoggerInfo>> current_data_loggers_;  // For ordered traversal
};

// FrequentDataLogger: A utility class for throttled logging of high-frequency data.
//
// Inherits aggregation from FrequentDataAggregator and outputs reports through LogAdapter.
//
// Example:
//   FrequentDataLogger::SetGlobalLogAdapter([](FrequentDataLogLevel level, const std::string& msg) {
//     LOG(LS_INFO) << msg;
//   });
//   FrequentDataLogger logger;
//   logger.LogData("packet_seq", now_ms) << seq_num << " ";
//
class FrequentDataLogger : public FrequentDataAggregator {
 public:
  // Set the global log adapter for all FrequentDataLogger instances.
  // Should be called once at application startup before any logging occurs.
  // @param log_adapter  Custom logging function (can be nullptr to disable logging)
  static void SetGlobalLogAdapter(LogAdapter log_adapter);

 protected:
  void OnReport(FrequentDataLogLevel level,
                const std::string& tag,
                FrequentDataReportType type,
                const std::string& message) override;

 private:
  static LogAdapter s_log_adapter_;               // Global log adapter shared by all instances
};

// FrequentDataStatsCollector: Collects aggregated reports as strings without logging.
//
// Example:
//   FrequentDataStatsCollector collector;
//   collector.LogIntervalData("bytes_sent", now_ms, total_bytes);
//   auto reports = collector.TakeReports();
//   // reports["bytes_sent"].buffered_data_report   (may be empty)
//   // reports["bytes_sent"].interval_stats_report  (may be empty)
//   // reports["bytes_sent"].warning_message        (may be empty)
//   // reports["bytes_sent"].error_message          (may be empty)
//
class FrequentDataStatsCollector : public FrequentDataAggregator {
 public:
  struct TagReport {
    std::string buffered_data_report;
    std::string interval_stats_report;
    std::string warning_message;
    std::string error_message;
  };

  // Each tag retains at most one TagReport until TakeReports() is called.
  using ReportsByTag = std::unordered_map<std::string, TagReport>;

  // Returns and clears pending reports grouped by data_logger_tag.
  ReportsByTag TakeReports();

 protected:
  void OnReport(FrequentDataLogLevel level,
                const std::string& tag,
                FrequentDataReportType type,
                const std::string& message) override;

 private:
  ReportsByTag reports_;  // Protected by data_loggers_mutex_
};

}  // namespace webrtc
#endif  // FREQUENT_DATA_LOGGER_H_
