#include "extension_api.h"
#include <stdio.h>

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
