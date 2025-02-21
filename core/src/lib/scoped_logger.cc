#include "scoped_logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

// logger
spdlog::logger& ScopedLogger::here(libbareos::source_location location) {
  auto name = LoggerName(location.file_name());
  auto it = loggers_.find(name);
  if (it == loggers_.end()) {
    auto& logger = *loggers_.emplace(name, spdlog::stdout_color_mt(name)).first->second;
    logger.set_level(GetLevel(logger.name()));
    return logger;
  }
  else {
    return *it->second;
  }
}

// level
spdlog::level::level_enum ScopedLogger::GetLevel(const std::string& filepath) {
  auto it = levels_.upper_bound(filepath);
  if (it != levels_.begin()) {
    it = std::prev(it);
    if (filepath.compare(0, it->first.size(), it->first) == 0) {
      return it->second;
    }
  }
  return spdlog::level::info;
}
void ScopedLogger::SetLevel(const std::string& prefix, spdlog::level::level_enum level) {
  // override subgroups
  for (auto it = levels_.lower_bound(prefix); it != levels_.end(); it = levels_.erase(it)) {
    if (it->first.compare(0, prefix.size(), prefix) != 0) {
        break;
    }
  }
  levels_[prefix] = level;
  // set logger
  for (auto it = loggers_.lower_bound(prefix); it != loggers_.end(); ++it) {
    if (it->first.compare(0, prefix.size(), prefix) != 0) {
        return;
    }
    it->second->set_level(level);
  }
}
void ScopedLogger::SetLevel(spdlog::level::level_enum level) {
  SetLevel(std::string(), level);
}

std::string ScopedLogger::LoggerName(std::string_view filename) {
  auto index = filename.find("src/");
  if (index != std::string::npos) {
    return std::string(filename.substr(index + std::string_view("src/").length()));
  }
  else {
    return std::string(filename);
  }
}
