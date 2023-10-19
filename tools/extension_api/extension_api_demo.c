#include "extension_api_demo.h"
#include <stdio.h>
#include <stdlib.h>

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

const char *duckdb_init(duckdb_extension_api *env) {
	const char *name = "hello world";

	duckdb_logical_type field_type = duckdb_create_logical_type(env, DUCKDB_TYPE_VARCHAR);
	printf("field_type: %p\n", (void *)field_type);

	duckdb_logical_type res = duckdb_create_struct_type(env, &field_type, &name, 1);
	printf("res: %p\n", (void *)res);

	printf("duckdb version: %s\n", duckdb_library_version(env));

	duckdb_destroy_logical_type(env, &field_type);
	duckdb_destroy_logical_type(env, &res);
	duckdb_free(env, env);

	return "version one";
}
