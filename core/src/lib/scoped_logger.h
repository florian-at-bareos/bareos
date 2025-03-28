/*
   BAREOS® - Backup Archiving REcovery Open Sourced

   Copyright (C) 2000-2011 Free Software Foundation Europe e.V.
   Copyright (C) 2016-2025 Bareos GmbH & Co. KG

   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.
*/

#ifndef BAREOS_LIB_LOGGER_H_
#define BAREOS_LIB_LOGGER_H_

#include "source_location.h"
#define SPDLOG_COMPILED_LIB
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <shared_mutex>
#include <map>
#include <filesystem>
#include <variant>


template<class T, typename Enable = void>
struct fixed_format {
  using type = std::remove_reference_t<T>;
};
template<class T>
struct fixed_format<T*, void> {
  using type = const void*;
};
template<>
struct fixed_format<const char*, void> {
  using type = const char*;
};
template<class T>
struct fixed_format<T, std::enable_if_t<std::is_function_v<std::remove_pointer_t<T>>>> {
  using type = const void*;
};
template<class T>
struct fixed_format<T, std::enable_if_t<std::is_enum_v<T>>> {
  using type = std::underlying_type_t<T>;
};
template<class T>
struct fixed_format<T, std::enable_if_t<!std::is_enum_v<T> && !std::is_pointer_v<T> && !std::is_function_v<T>>> {
  using type = std::remove_reference_t<T>;
};

template<class T>
const auto* FixFormatArg(const T* arg) {
  if constexpr(std::is_same_v<const T*, const char*>) {
    return arg;
  }
  else {
    return static_cast<const void*>(arg);
  }
}
template<class T, typename = std::enable_if_t<std::is_function_v<std::remove_pointer_t<T>>>>
const void* FixFormatArg(const T& arg) {
  return (const void*)(arg);
}
template<class T, typename = std::enable_if_t<std::is_enum_v<T>>>
auto FixFormatArg(T arg) {
  return static_cast<std::underlying_type_t<T>>(arg);
}
template<class T, typename = std::enable_if_t<!std::is_enum_v<T> && !std::is_pointer_v<T> && !std::is_function_v<T>>>
const auto& FixFormatArg(const T& arg) {
  return arg;
}

struct ConsoleSink {};
struct FileSink {
  std::filesystem::path path;
};
using Sink = std::variant<ConsoleSink, FileSink>;

class Logger : public spdlog::logger {
public:
  Logger(const std::string& name, spdlog::level::level_enum level = spdlog::level::debug);
  Logger(const std::string& name, spdlog::sink_ptr, spdlog::level::level_enum level = spdlog::level::debug);

  template<class... Args>
  void debug(spdlog::format_string_t<typename fixed_format<Args>::type...> str, Args&&... args) {
    spdlog::logger::debug(str, FixFormatArg(args)...);
  }
  template<class... Args>
  void debug(const char* str, Args&&... args) {
    spdlog::logger::debug(fmt::runtime(str), FixFormatArg(args)...);
  }
};

class ScopedLogger {
public:
  // get
  static Logger& GetLogger(libbareos::source_location location = libbareos::source_location::current());

  // level
  static spdlog::level::level_enum GetLevel(const std::string& filepath);
  static void SetLevel(spdlog::level::level_enum level, const std::string& affected_path = "");

  // sink
  static spdlog::sink_ptr GetSink(const Sink& sink);
  static std::vector<spdlog::sink_ptr> GetSinks(const std::string& filepath);
  static void SetFileSink(const std::filesystem::path& filepath, const std::string& affected_path = "");
private:
  static void ForeachLogger(const std::string& affected_path, std::function<void(Logger&)> func);

  static inline std::map<std::string, Logger> logger_map_;
  static inline std::map<std::string, spdlog::level::level_enum> level_map_;
  static inline std::map<std::string, std::vector<Sink>> sink_map_;

  static inline std::map<std::filesystem::path, spdlog::sink_ptr> file_sinks_;
  static inline spdlog::sink_ptr console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
};

class LoggerHandle {
public:
  Logger& get(libbareos::source_location location = libbareos::source_location::current());
private:
  std::vector<std::pair<const char*, Logger*>> loggers_;
};

static LoggerHandle source_logger;

#endif  // BAREOS_LIB_LOGGER_H_
