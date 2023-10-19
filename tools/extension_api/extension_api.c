#include "extension_api.h"

duckdb_logical_type duckdb_create_struct_type(duckdb_extension_api *env, duckdb_logical_type *types, const char **names,
                                              idx_t n_members) {
	return env->duckdb_create_struct_type(types, names, n_members);
}

duckdb_logical_type duckdb_create_logical_type(duckdb_extension_api *env, duckdb_type type) {
	return env->duckdb_create_logical_type(type);
}

void duckdb_destroy_logical_type(duckdb_extension_api *env, duckdb_logical_type *type) {
	env->duckdb_destroy_logical_type(type);
}

void duckdb_free(duckdb_extension_api *env, void *ptr) {
	env->duckdb_free(ptr);
}

const char *duckdb_library_version(duckdb_extension_api *env) {
	return env->duckdb_library_version();
}
