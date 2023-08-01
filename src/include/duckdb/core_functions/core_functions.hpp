//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/core_functions/core_functions.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/common.hpp"
#include "duckdb/core_functions/function_helpers.hpp"
#include "duckdb/core_functions/function_list.hpp"

namespace duckdb {

class Catalog;
struct CatalogTransaction;

struct CoreFunctions {
	static void RegisterFunctions(Catalog &catalog, CatalogTransaction transaction,
	                              StaticFunctionDefinition *functions);
	static void RegisterFunctions(Catalog &catalog, CatalogTransaction transaction);
};

} // namespace duckdb
