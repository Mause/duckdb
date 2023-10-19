#include "duckdb_extension_api.h"
#include <stdio.h>
#include <inttypes.h>
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

void duckdb_init(duckdb_extension_api *env) {
	const char *name = "hello world";

	duckdb_logical_type field_type = duckdb_create_logical_type(env, DUCKDB_TYPE_VARCHAR);
	printf("field_type: %" PRIu64 "\n", field_type);

	duckdb_logical_type res = duckdb_create_struct_type(env, &field_type, &name, 1);
	printf("res: %" PRIu64 "\n", res);

	duckdb_destroy_logical_type(env, &field_type);
	duckdb_destroy_logical_type(env, &res);
	free(env); // TODO: duckdb_free
}
