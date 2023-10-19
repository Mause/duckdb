#pragma once

#include "duckdb.hpp"
#include "duckdb_extension_api.h"

namespace duckdb {

class ExtensionApiExtension : public Extension {
public:
	void Load(DuckDB &db) override;
	std::string Name() override;
};

} // namespace duckdb
