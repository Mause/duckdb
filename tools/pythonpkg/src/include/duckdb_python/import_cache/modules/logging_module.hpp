
//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb_python/import_cache/modules/logging_module.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb_python/import_cache/python_import_cache_item.hpp"

namespace duckdb {

struct LoggingLoggerCacheItem : public PythonImportCacheItem {

public:
	LoggingLoggerCacheItem(optional_ptr<PythonImportCacheItem> parent)
	    : PythonImportCacheItem("Logger", parent), info("info", this), error("error", this), warning("warning", this) {
	}
	~LoggingLoggerCacheItem() override {
	}

	PythonImportCacheItem info;
	PythonImportCacheItem error;
	PythonImportCacheItem warning;
};

struct LoggingCacheItem : public PythonImportCacheItem {

public:
	static constexpr const char *Name = "logging";

public:
	LoggingCacheItem() : PythonImportCacheItem("logging"), getLogger("getLogger", this), Logger(this) {
	}
	~LoggingCacheItem() override {
	}

	PythonImportCacheItem getLogger;
	LoggingLoggerCacheItem Logger;
};

} // namespace duckdb
