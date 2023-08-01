#pragma once

// Scalar Function
#define DUCKDB_SCALAR_FUNCTION_BASE(_PARAM, _NAME)                                                                     \
	{ _NAME, _PARAM::Parameters, _PARAM::Description, _PARAM::Example, _PARAM::GetFunction, nullptr, nullptr, nullptr }
#define DUCKDB_SCALAR_FUNCTION(_PARAM)       DUCKDB_SCALAR_FUNCTION_BASE(_PARAM, _PARAM::Name)
#define DUCKDB_SCALAR_FUNCTION_ALIAS(_PARAM) DUCKDB_SCALAR_FUNCTION_BASE(_PARAM::ALIAS, _PARAM::Name)
// Scalar Function Set
#define DUCKDB_SCALAR_FUNCTION_SET_BASE(_PARAM, _NAME)                                                                 \
	{ _NAME, _PARAM::Parameters, _PARAM::Description, _PARAM::Example, nullptr, _PARAM::GetFunctions, nullptr, nullptr }
#define DUCKDB_SCALAR_FUNCTION_SET(_PARAM)       DUCKDB_SCALAR_FUNCTION_SET_BASE(_PARAM, _PARAM::Name)
#define DUCKDB_SCALAR_FUNCTION_SET_ALIAS(_PARAM) DUCKDB_SCALAR_FUNCTION_SET_BASE(_PARAM::ALIAS, _PARAM::Name)
// Aggregate Function
#define DUCKDB_AGGREGATE_FUNCTION_BASE(_PARAM, _NAME)                                                                  \
	{ _NAME, _PARAM::Parameters, _PARAM::Description, _PARAM::Example, nullptr, nullptr, _PARAM::GetFunction, nullptr }
#define DUCKDB_AGGREGATE_FUNCTION(_PARAM)       DUCKDB_AGGREGATE_FUNCTION_BASE(_PARAM, _PARAM::Name)
#define DUCKDB_AGGREGATE_FUNCTION_ALIAS(_PARAM) DUCKDB_AGGREGATE_FUNCTION_BASE(_PARAM::ALIAS, _PARAM::Name)
// Aggregate Function Set
#define DUCKDB_AGGREGATE_FUNCTION_SET_BASE(_PARAM, _NAME)                                                              \
	{ _NAME, _PARAM::Parameters, _PARAM::Description, _PARAM::Example, nullptr, nullptr, nullptr, _PARAM::GetFunctions }
#define DUCKDB_AGGREGATE_FUNCTION_SET(_PARAM)       DUCKDB_AGGREGATE_FUNCTION_SET_BASE(_PARAM, _PARAM::Name)
#define DUCKDB_AGGREGATE_FUNCTION_SET_ALIAS(_PARAM) DUCKDB_AGGREGATE_FUNCTION_SET_BASE(_PARAM::ALIAS, _PARAM::Name)
#define FINAL_FUNCTION                                                                                                 \
	{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
