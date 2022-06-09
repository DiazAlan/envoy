#pragma once

#include <string>

#include "source/common/common/macros.h"

#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "spdlog/spdlog.h"

namespace Envoy {

using SpdLoggerSharedPtr = std::shared_ptr<spdlog::logger>;
using FancyMap = absl::flat_hash_map<std::string, SpdLoggerSharedPtr>;
using FancyMapPtr = std::shared_ptr<FancyMap>;
using FancyLogLevelMap = absl::flat_hash_map<std::string, spdlog::level::level_enum>;

constexpr int kLogLevelMax = 6; // spdlog level is in [0, 6].
constexpr int kLogLevelMin = 0;

/**
 * Data struct that stores the necessary verbosity log update info.
 */
struct VerbosityLogUpdateInfo final {
  const std::string update_pattern;
  const bool update_is_path; // i.e. it contains a path separator.
  const spdlog::level::level_enum log_level;

  VerbosityLogUpdateInfo(absl::string_view update_pattern, bool update_is_path,
                         spdlog::level::level_enum log_level)
      : update_pattern(std::string(update_pattern)), update_is_path(update_is_path),
        log_level(log_level) {}
};

/**
 * Stores the lock and functions used by Fancy Logger's macro so that we don't need to declare
 * them globally. Functions are provided to initialize a logger, set log level, flush a logger.
 */
class FancyContext {
public:
  /**
   * Gets a logger from map given a key (e.g. file name).
   */
  SpdLoggerSharedPtr getFancyLogEntry(std::string key) ABSL_LOCKS_EXCLUDED(fancy_log_lock_);

  /**
   * Initializes Fancy Logger, gets log level from setting vector, and registers it in global
   * map if not done.
   */
  void initFancyLogger(std::string key, std::atomic<spdlog::logger*>& logger)
      ABSL_LOCKS_EXCLUDED(fancy_log_lock_);

  /**
   * Sets log level. If not found, return false.
   */
  bool setFancyLogger(std::string key, spdlog::level::level_enum log_level)
      ABSL_LOCKS_EXCLUDED(fancy_log_lock_);

  /**
   * Sets the default logger level and format when updating context. It should only be used in
   * Context, otherwise the fancy_default_level will possibly be inconsistent with the actual
   * logger level.
   */
  void setDefaultFancyLevelFormat(spdlog::level::level_enum level, std::string format)
      ABSL_LOCKS_EXCLUDED(fancy_log_lock_);

  /**
   * Lists keys and levels of all loggers in a string for admin page usage.
   */
  std::string listFancyLoggers() ABSL_LOCKS_EXCLUDED(fancy_log_lock_);

  /**
   * Sets the levels of all loggers.
   */
  void setAllFancyLoggers(spdlog::level::level_enum level) ABSL_LOCKS_EXCLUDED(fancy_log_lock_);

  /**
   * Obtain a map from logger key to log level. Useful for testing, e.g. in macros such as
   * EXPECT_LOG_CONTAINS_ALL_OF_HELPER.
   */
  FancyLogLevelMap getAllFancyLogLevelsForTest() ABSL_LOCKS_EXCLUDED(fancy_log_lock_);

  /**
   * Updates the all the loggers based on the verbosity updates <(file, level) ...>.
   * It supports file basename and glob "*" and "?" pattern, eg. ("foo", 2), ("foo/b*", 3)
   * Patterns including a slash character are matched against full path names, while those
   * without are matched against base names (by removing one suffix) only.
   *
   * It will store the current verbosity updates and clear all previous modifications for
   * future check when initializing a new logger.
   *
   * Files are matched against globs in updates in order, and the first match determines
   * the verbosity level.
   *
   * Files which do not match any pattern use the value of default log level from Context.
   */
  void updateVerbositySetting(const std::vector<std::pair<absl::string_view, int>>& updates)
      ABSL_LOCKS_EXCLUDED(fancy_log_lock_);

  /**
   * Check if a string matches a glob pattern. It only supports "*" and "?" wildcards,
   * and wildcards may match /. No support for bracket expressions [...].
   */
  static bool safeFileNameMatch(absl::string_view pattern, absl::string_view str);

private:
  /**
   * Initializes sink for the initialization of loggers, needed only in benchmark test.
   */
  void initSink();

  /**
   * Creates a logger given key, and add it to map. Log level is from getLogLevel.
   * Key is the log component name, e.g. file name now.
   */
  spdlog::logger* createLogger(const std::string& key)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(fancy_log_lock_);

  /**
   * Append verbosity level updates to the VerbosityLogUpdateInfo vector.
   */
  void appendVerbosityLogUpdate(absl::string_view update_pattern,
                                spdlog::level::level_enum log_level)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(fancy_log_lock_);

  /**
   * Returns the current log level of `file`. Default log level is used if there is no
   * match in log_update_info_.
   */
  spdlog::level::level_enum getLogLevel(absl::string_view file) const
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(fancy_log_lock_);

  /**
   * Lock for the following global map and update vector (not for the corresponding loggers).
   */
  absl::Mutex fancy_log_lock_;

  /**
   * Map that stores <key, logger> pairs, key can be the file name.
   */
  FancyMapPtr fancy_log_map_ ABSL_GUARDED_BY(fancy_log_lock_) = std::make_shared<FancyMap>();

  /**
   * Vector that stores <update, level> pairs, key can be the file basename or glob expressions.
   */
  std::vector<VerbosityLogUpdateInfo> log_update_info_ ABSL_GUARDED_BY(fancy_log_lock_);
};

FancyContext& getFancyContext();

#define FANCY_KEY std::string(__FILE__)

/**
 * Macro for fancy logger.
 * Uses a global map to store logger and take use of thread-safe spdlog::logger.
 * The local pointer is used to avoid another load() when logging. Here we use
 * spdlog::logger* as atomic<shared_ptr> is a C++20 feature.
 */
#define FANCY_LOG(LEVEL, ...)                                                                      \
  do {                                                                                             \
    static std::atomic<spdlog::logger*> flogger{0};                                                \
    spdlog::logger* local_flogger = flogger.load(std::memory_order_relaxed);                       \
    if (!local_flogger) {                                                                          \
      ::Envoy::getFancyContext().initFancyLogger(FANCY_KEY, flogger);                              \
      local_flogger = flogger.load(std::memory_order_relaxed);                                     \
    }                                                                                              \
    if (ENVOY_LOG_COMP_LEVEL(*local_flogger, LEVEL)) {                                             \
      local_flogger->log(spdlog::source_loc{__FILE__, __LINE__, __func__},                         \
                         ENVOY_SPDLOG_LEVEL(LEVEL), __VA_ARGS__);                                  \
    }                                                                                              \
  } while (0)

/**
 * Convenient macro for connection log.
 */
#define FANCY_CONN_LOG(LEVEL, FORMAT, CONNECTION, ...)                                             \
  FANCY_LOG(LEVEL, "[C{}] " FORMAT, (CONNECTION).id(), ##__VA_ARGS__)

/**
 * Convenient macro for stream log.
 */
#define FANCY_STREAM_LOG(LEVEL, FORMAT, STREAM, ...)                                               \
  FANCY_LOG(LEVEL, "[C{}][S{}] " FORMAT, (STREAM).connection() ? (STREAM).connection()->id() : 0,  \
            (STREAM).streamId(), ##__VA_ARGS__)

/**
 * Convenient macro for log flush.
 */
#define FANCY_FLUSH_LOG()                                                                          \
  do {                                                                                             \
    SpdLoggerSharedPtr p = ::Envoy::getFancyContext().getFancyLogEntry(FANCY_KEY);                 \
    if (p) {                                                                                       \
      p->flush();                                                                                  \
    }                                                                                              \
  } while (0)

} // namespace Envoy
