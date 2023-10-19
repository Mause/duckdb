#pragma once

#include "duckdb.hpp"

namespace duckdb {

typedef struct {
	duckdb_logical_type (*duckdb_create_struct_type)(duckdb_logical_type *members, const char **names, idx_t n_members);
	duckdb_logical_type (*duckdb_create_logical_type)(duckdb_logical_type *members, duckdb_type type);
	void (*duckdb_destroy_logical_type)(duckdb_logical_type member);
} duckdb_extension_api;

class ExtensionApiExtension : public Extension {
public:
	void Load(DuckDB &db) override;
	std::string Name() override;
};

} // namespace duckdb
