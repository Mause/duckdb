#pragma once

#include "duckdb/core_functions/function_list.hpp"

namespace duckdb {
class OperatorFunctions {
public:
	static StaticFunctionDefinition *GetFunctions();
};
} // namespace duckdb
