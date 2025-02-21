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
// #define SPDLOG_COMPILED_LIB
#include <spdlog/spdlog.h>
#include <string>
#include <map>

class ScopedLogger {
public:
  // logger 
  spdlog::logger& here(libbareos::source_location location = libbareos::source_location::current());
  
  // level
  spdlog::level::level_enum GetLevel(const std::string& filepath);
  void SetLevel(const std::string& prefix, spdlog::level::level_enum level);
  void SetLevel(spdlog::level::level_enum level);
private:
  static std::string LoggerName(std::string_view filename);

  std::map<std::string, std::shared_ptr<spdlog::logger>> loggers_;
  std::map<std::string, spdlog::level::level_enum> levels_;
};

#endif  // BAREOS_LIB_LOGGER_H_
