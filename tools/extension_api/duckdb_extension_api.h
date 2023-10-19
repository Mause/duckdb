#pragma once
#include <stdint.h>

typedef uint64_t idx_t;

typedef enum DUCKDB_TYPE {
	DUCKDB_TYPE_INVALID = 0,
	// bool
	DUCKDB_TYPE_BOOLEAN,
	// int8_t
	DUCKDB_TYPE_TINYINT,
	// int16_t
	DUCKDB_TYPE_SMALLINT,
	// int32_t
	DUCKDB_TYPE_INTEGER,
	// int64_t
	DUCKDB_TYPE_BIGINT,
	// uint8_t
	DUCKDB_TYPE_UTINYINT,
	// uint16_t
	DUCKDB_TYPE_USMALLINT,
	// uint32_t
	DUCKDB_TYPE_UINTEGER,
	// uint64_t
	DUCKDB_TYPE_UBIGINT,
	// float
	DUCKDB_TYPE_FLOAT,
	// double
	DUCKDB_TYPE_DOUBLE,
	// duckdb_timestamp, in microseconds
	DUCKDB_TYPE_TIMESTAMP,
	// duckdb_date
	DUCKDB_TYPE_DATE,
	// duckdb_time
	DUCKDB_TYPE_TIME,
	// duckdb_interval
	DUCKDB_TYPE_INTERVAL,
	// duckdb_hugeint
	DUCKDB_TYPE_HUGEINT,
	// const char*
	DUCKDB_TYPE_VARCHAR,
	// duckdb_blob
	DUCKDB_TYPE_BLOB,
	// decimal
	DUCKDB_TYPE_DECIMAL,
	// duckdb_timestamp, in seconds
	DUCKDB_TYPE_TIMESTAMP_S,
	// duckdb_timestamp, in milliseconds
	DUCKDB_TYPE_TIMESTAMP_MS,
	// duckdb_timestamp, in nanoseconds
	DUCKDB_TYPE_TIMESTAMP_NS,
	// enum type, only useful as logical type
	DUCKDB_TYPE_ENUM,
	// list type, only useful as logical type
	DUCKDB_TYPE_LIST,
	// struct type, only useful as logical type
	DUCKDB_TYPE_STRUCT,
	// map type, only useful as logical type
	DUCKDB_TYPE_MAP,
	// duckdb_hugeint
	DUCKDB_TYPE_UUID,
	// union type, only useful as logical type
	DUCKDB_TYPE_UNION,
	// duckdb_bit
	DUCKDB_TYPE_BIT,
} duckdb_type;

typedef struct _duckdb_logical_type {
	void *__lglt;
} * duckdb_logical_type;

typedef struct {
	duckdb_logical_type (*duckdb_create_struct_type)(duckdb_logical_type *types, const char **names, idx_t n_members);
	duckdb_logical_type (*duckdb_create_logical_type)(duckdb_type type);
	void (*duckdb_destroy_logical_type)(duckdb_logical_type *member);
	void (*duckdb_free)(void *ptr);
	const char *(*duckdb_library_version)();
} duckdb_extension_api;

duckdb_logical_type duckdb_create_logical_type(duckdb_extension_api *env, duckdb_type type);
duckdb_logical_type duckdb_create_struct_type(duckdb_extension_api *env, duckdb_logical_type *types, const char **names,
                                              idx_t n_members);
void duckdb_destroy_logical_type(duckdb_extension_api *env, duckdb_logical_type *type);
void duckdb_free(duckdb_extension_api *env, void *ptr);
const char *duckdb_library_version(duckdb_extension_api *env);
