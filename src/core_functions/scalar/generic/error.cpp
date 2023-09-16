#include "duckdb/core_functions/scalar/generic_functions.hpp"
#include <iostream>

namespace duckdb {

struct ErrorOperator {
	template <class TA, class TR>
	static inline TR Operation(const TA &input) {
		throw Exception(input.GetString());
	}
};

ScalarFunction ErrorFun::GetFunction() {
	auto fun = ScalarFunction({LogicalType::VARCHAR}, LogicalType::BOOLEAN,
	                          ScalarFunction::UnaryFunction<string_t, bool, ErrorOperator>);
	// Set the function with side effects to avoid the optimization.
	fun.side_effects = FunctionSideEffects::HAS_SIDE_EFFECTS;
	return fun;
}

static bool GetAssertionsEnabled() {
	try {
		D_ASSERT(0);
	} catch (const InternalException& e) {
		return true;
	}
	return false;
}

ScalarFunction AssertionsEnabledCheckFun::GetFunction() {
	return ScalarFunction({}, LogicalType::BOOLEAN, [](DataChunk &input, ExpressionState &state, Vector &result) {
		result.Reference(Value::BOOLEAN(GetAssertionsEnabled()));
	});
}

class CompileFlagsFunctionData : public FunctionData {
public:
	bool done = false;
};

static unique_ptr<FunctionData> Binder(ClientContext&, TableFunctionBindInput&, vector<LogicalType> &column_types, vector<string> &names) {
	names.push_back("key");
	column_types.emplace_back(LogicalTypeId::VARCHAR);

	names.push_back("value");
	column_types.emplace_back(LogicalType::UNION({{"boolean", LogicalTypeId::BOOLEAN}}));

	return nullptr;
}

class CompileFlagsState : public GlobalTableFunctionState {
public:
	bool done;
};

static bool GetWasmLoadableExtensions() {
#ifdef WASM_LOADABLE_EXTENSIONS
	return true;
#else
	return false;
#endif
}

static void SetRow(DataChunk &chunk, int row, const char *key, const Value &value) {
	chunk.SetValue(0, row, Value(key));
	chunk.SetValue(1, row, value);
}

static void CompileFlags(ClientContext&, TableFunctionInput& input, DataChunk& chunk) {
	int row = 0;

	CompileFlagsState &state = input.global_state->Cast<CompileFlagsState>();

	if (state.done) {
		chunk.SetCardinality(0);
		return;
	}
	state.done = true;

	SetRow(chunk, row++, "assertions_enabled", Value::BOOLEAN(GetAssertionsEnabled()));
	SetRow(chunk, row++, "wasm_build", Value::BOOLEAN(GetWasmLoadableExtensions()));

	chunk.SetCardinality(row);
}

unique_ptr<GlobalTableFunctionState> GlobalIniter(ClientContext&, TableFunctionInitInput&) {
	auto ptr = make_uniq<CompileFlagsState>();
	ptr->done = false;
	return std::move(ptr);
}

TableFunction CompileFlagsFun::GetFunction() {
	return TableFunction({}, CompileFlags, Binder, GlobalIniter);
}

} // namespace duckdb
