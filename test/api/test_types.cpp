#include "test_helpers.hpp"
#include "duckdb/catalog/catalog_entry/scalar_function_catalog_entry.hpp"

#include <duckdb.hpp>

static void ExtractFunctionsFromSchema(ClientContext &context, SchemaCatalogEntry &schema,
                                       duckdb::vector<reference<FunctionEntry>> &entries) {
	schema.Scan(context, CatalogType::SCALAR_FUNCTION_ENTRY, [&](CatalogEntry &entry) {
		if ((StringUtil::Contains(entry.name, "__internal") || entry.name == "error")) {
			return;
		}
		auto scalar = dynamic_cast<ScalarFunctionCatalogEntry *>(&entry);
		if (scalar) {
			entries.push_back(*scalar);
		}
	});
}

TEST_CASE("ensure scalar functions have examples that work", "[types]") {
	DuckDB duckdb(nullptr);
	Connection connection(duckdb);
	auto &context = connection.context;

	duckdb::vector<reference<FunctionEntry>> entries;

	connection.BeginTransaction();

	connection.Query("CREATE TYPE mood AS ENUM ('sad', 'ok', 'happy', 'anxious');");

	auto schemas = Catalog::GetAllSchemas(*context);
	for (auto &schema : schemas) {
		ExtractFunctionsFromSchema(*context, schema.get(), entries);
	}

	/*
	SECTION("functions should have examples") {
	    for (auto &entry : entries) {
	        DYNAMIC_SECTION(entry.get().name) {
	            REQUIRE(!entry.get().example.empty());
	        }
	    }
	}
	*/

	SECTION("function examples should be executable") {
		for (auto &entry : entries) {
			string &example = entry.get().example;
			if (!example.empty()) {
				DYNAMIC_SECTION(entry.get().name) {
					auto res = connection.Query("SELECT ['hello', 'world'] as l, l as col, " + example);
					if (res->HasError()) {
						res->ThrowError();
					} else {
						INFO(res->ToString());
					}
				}
			}
		}
	}
}
