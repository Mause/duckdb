#pragma once
#include "duckdb_extension_api.h"

duckdb_logical_type duckdb_create_logical_type(duckdb_extension_api *env, duckdb_type type);
duckdb_logical_type duckdb_create_struct_type(duckdb_extension_api *env, duckdb_logical_type *types, const char **names,
                                              idx_t n_members);
void duckdb_destroy_logical_type(duckdb_extension_api *env, duckdb_logical_type *type);
void duckdb_free(duckdb_extension_api *env, void *ptr);
const char *duckdb_library_version(duckdb_extension_api *env);
