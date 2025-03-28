#include "scoped_logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

Logger::Logger(const std::string& name, spdlog::level::level_enum level)
  : Logger(name, std::make_shared<spdlog::sinks::stdout_color_sink_mt>(), level) {}
Logger::Logger(const std::string& name, spdlog::sink_ptr sink, spdlog::level::level_enum level)
  : spdlog::logger(name, std::move(sink)) {
  set_level(level);
}

static std::filesystem::path Normalized(std::filesystem::path path) {
  path.replace_extension("");
  std::string str = path.string();
  size_t index1 = str.find("src/");
  if (index1 != std::string::npos) {
    return str.substr(index1 + std::string("src/").size());
  }
  size_t index2 = str.find("src\\");
  if (index2 != std::string::npos) {
    return str.substr(index2 + std::string("src\\").size());
  }
  return path;
}

Logger& ScopedLogger::GetLogger(libbareos::source_location location) {
  std::filesystem::path normalized = Normalized(location.file_name());
  std::string logger_name = normalized.string() + "/" + location.function_name();
  auto it = logger_map_.find(logger_name);
  if (it != logger_map_.end()) {
    return it->second;
  }
  else {
    auto [emplace_it, success] = logger_map_.emplace(logger_name, Logger(logger_name, console_sink_, GetLevel(logger_name)));
    return emplace_it->second;
  }
}

spdlog::level::level_enum ScopedLogger::GetLevel(const std::filesystem::path& filepath) {
  std::filesystem::path normalized = Normalized(filepath);
  auto it = level_map_.upper_bound(filepath);
  if (it != level_map_.begin()) {
    it = std::prev(it);
    if (std::filesystem::relative(filepath, it->first).begin()->string() != "..") {
      return it->second;
    }
  }
  auto root_it = level_map_.find("");
  if (root_it == level_map_.end()) {
    level_map_[""] = spdlog::level::debug;
    return spdlog::level::debug;
  }
  return root_it->second;
}
void ScopedLogger::SetLevel(spdlog::level::level_enum level, const std::filesystem::path& scope) {
  for (auto it = level_map_.lower_bound(scope); it != level_map_.end(); it = level_map_.erase(it)) {
    if (std::filesystem::relative(it->first, scope).begin()->string() == "..") {
      break;
    }
  }
  level_map_[scope] = level;
}

// spdlog::sink_ptr ScopedLogger::GetSink(const Sink& sink) {
//   if (std::holds_alternative<FileSink>(sink)) {
//     const auto& filepath = std::get<FileSink>(sink).path;
//     auto it = file_sinks_.find(filepath);
//     if (it != file_sinks_.end()) {
//       return it->second;
//     }
//     else {
//       auto [emplace_it, emplace_success] = file_sinks_.emplace(filepath, std::make_shared<spdlog::sinks::basic_file_sink_mt>(filepath));
//       return emplace_it->second;
//     }
//   }
//   else if (std::holds_alternative<ConsoleSink>(sink)) {
//     return console_sink_;
//   }
//   throw std::bad_variant_access();
// }

// std::vector<spdlog::sink_ptr> ScopedLogger::GetSinks(const std::string& filepath) {
//   std::vector<spdlog::sink_ptr> sinks;
//   for (auto it = sink_map_.lower_bound(affected_path); it != level_map_.end(); it = level_map_.erase(it)) {
//     if (it->first.compare(0, level_map_.size(), affected_path) != 0) {
//         break;
//     }
//   }
//   level_map_[affected_path] = level;
// }

// void ScopedLogger::SetFileSink(const std::filesystem::path& filepath, const std::string& affected_path) {
//   auto& sinks = sink_map_[affected_path];
//   for (auto& sink : sinks) {
//     if (std::holds_alternative<FileSink>(sink) && std::get<FileSink>(sink).path == filepath) {
//       return;
//     }
//   }
//   sinks.emplace_back(FileSink({ filepath }));
// }

void ScopedLogger::ForeachLogger(const std::filesystem::path& scope, std::function<void(Logger&)> func) {
  for (auto it = logger_map_.lower_bound(scope); it != logger_map_.end(); ++it) {
    if (std::filesystem::relative(it->first, scope).begin()->string() == "..") {
      return;
    }
    func(it->second);
  }
}

Logger& LoggerHandle::get(libbareos::source_location location) {
  for (auto [func_name, func_logger] : loggers_) {
    if (func_name == location.function_name()) {
      return *func_logger;
    }
  }
  loggers_.emplace_back(location.function_name(), &ScopedLogger::GetLogger(location));
  return *loggers_.back().second;
}