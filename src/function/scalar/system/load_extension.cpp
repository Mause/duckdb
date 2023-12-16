#include "duckdb/function/scalar/generic_functions.hpp"

#include "duckdb.hpp"
#include "duckdb.h"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/parser/parsed_data/create_scalar_function_info.hpp"

#include "duckdb/main/client_context.hpp"
#include "duckdb/catalog/catalog.hpp"
#include "duckdb/common/dl.hpp"

namespace duckdb {

static void load_extension_function(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &input_vector = args.data[0];

	UnaryExecutor::Execute<string_t, string_t>(input_vector, result, args.size(), [&](string_t input) {
		auto extension_path = input.GetString();

		auto extension = dlopen(extension_path.c_str(), RTLD_LAZY);
		if (extension == nullptr) {
			throw InvalidInputException("Unable to load library");
		}

		auto init = (const char *(*)(duckdb_extension_api *))dlsym(extension, "duckdb_init");
		if (init == nullptr) {
			throw InvalidInputException("Unable to load initialisation function");
		}

		auto ext_api = (duckdb_extension_api *)duckdb_malloc(sizeof(duckdb_extension_api));

		ext_api->duckdb_create_struct_type = duckdb_create_struct_type;
		ext_api->duckdb_create_logical_type = duckdb_create_logical_type;
		ext_api->duckdb_destroy_logical_type = duckdb_destroy_logical_type;
		ext_api->duckdb_free = duckdb_free;
		ext_api->duckdb_library_version = duckdb_library_version;

		auto version = string(init(ext_api));

		auto output = StringVector::AddString(result, StringUtil::Format("extension loaded: %s", version));

		return output;
	});
}

void LoadExtensionFunction::RegisterFunction(BuiltinFunctions &set) {
	ScalarFunction load_extension_func("load_extension", {LogicalType::VARCHAR}, LogicalType::VARCHAR,
	                                   load_extension_function);

	set.AddFunction(load_extension_func);
}

} // namespace duckdb
