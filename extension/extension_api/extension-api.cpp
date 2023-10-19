#define DUCKDB_EXTENSION_MAIN
#include "extension_api_extension.hpp"

#include "duckdb.hpp"
#include "duckdb.h"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/parser/parsed_data/create_scalar_function_info.hpp"
#include "duckdb/main/extension_util.hpp"

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

void ExtensionApiExtension::Load(DuckDB &db) {
	auto &db_instance = *db.instance;

	ScalarFunction load_extension_func("load_extension", {LogicalType::VARCHAR}, LogicalType::VARCHAR,
	                                   load_extension_function);

	ExtensionUtil::RegisterFunction(db_instance, load_extension_func);
}

std::string ExtensionApiExtension::Name() {
	return "extension_api";
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void extension_api_init(duckdb::DatabaseInstance &db) {
	duckdb::DuckDB db_wrapper(db);
	db_wrapper.LoadExtension<duckdb::ExtensionApiExtension>();
}

DUCKDB_EXTENSION_API const char *extension_api_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
