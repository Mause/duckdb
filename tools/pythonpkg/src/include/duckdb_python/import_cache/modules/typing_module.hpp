
//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb_python/import_cache/modules/typing_module.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb_python/import_cache/python_import_cache_item.hpp"

namespace duckdb {

struct TypingCacheItem : public PythonImportCacheItem {

public:
	static constexpr const char *Name = "typing";

public:
	TypingCacheItem() : PythonImportCacheItem("typing"), Union("Union", this), _GenericAlias("_GenericAlias", this) {
	}
	~TypingCacheItem() override {
	}

	PythonImportCacheItem Union;
	PythonImportCacheItem _GenericAlias;
};

} // namespace duckdb
