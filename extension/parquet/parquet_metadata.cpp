#include "parquet_metadata.hpp"

#include "parquet_statistics.hpp"

#include <sstream>

#ifndef DUCKDB_AMALGAMATION
#include "duckdb/common/multi_file_reader.hpp"
#include "duckdb/common/types/blob.hpp"
#include "duckdb/common/types/column/column_data_collection.hpp"
#include "duckdb/main/config.hpp"
#endif

namespace duckdb {

struct ParquetMetaDataBindData : public TableFunctionData {
	vector<LogicalType> return_types;
	vector<string> files;

public:
	bool Equals(const FunctionData &other_p) const override {
		auto &other = other_p.Cast<ParquetMetaDataBindData>();
		return other.return_types == return_types && files == other.files;
	}
};

struct ParquetMetaDataOperatorData : public GlobalTableFunctionState {
	explicit ParquetMetaDataOperatorData(ClientContext &context, const vector<LogicalType> &types)
	    : collection(context, types) {
	}

	idx_t file_index;
	ColumnDataCollection collection;
	ColumnDataScanState scan_state;

public:
	static void BindMetaData(vector<LogicalType> &return_types, vector<string> &names);
	static void BindSchema(vector<LogicalType> &return_types, vector<string> &names);
	static void BindFileLevelMetaData(vector<LogicalType> &return_types, vector<string> &names);

	void LoadFileMetaData(ClientContext &context, const vector<LogicalType> &return_types, const string &file_path);
	void LoadSchemaData(ClientContext &context, const vector<LogicalType> &return_types, const string &file_path);
	void LoadFileLevelMetadata(ClientContext &context, const vector<LogicalType> &return_types,
	                           const std::string &filename);

	static LogicalType GetType();
	static Value KeyValueToMap(const vector<duckdb_parquet::format::KeyValue> &key_value_metadata);
};

template <class T>
string ConvertParquetElementToString(T &&entry) {
	std::stringstream ss;
	ss << entry;
	return ss.str();
}

template <class T>
string PrintParquetElementToString(T &&entry) {
	std::stringstream ss;
	entry.printTo(ss);
	return ss.str();
}

template <class T>
Value ParquetElementString(T &&value, bool is_set) {
	if (!is_set) {
		return Value();
	}
	return Value(ConvertParquetElementToString(value));
}

template <class T>
Value ParquetElementInteger(T &&value, bool is_iset) {
	if (!is_iset) {
		return Value();
	}
	return Value::INTEGER(value);
}

template <class T>
Value ParquetElementBigint(T &&value, bool is_iset) {
	if (!is_iset) {
		return Value();
	}
	return Value::BIGINT(value);
}

void ParquetMetaDataOperatorData::BindMetaData(vector<LogicalType> &return_types, vector<string> &names) {
	names.emplace_back("file_name");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("row_group_id");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("row_group_num_rows");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("row_group_num_columns");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("row_group_bytes");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("column_id");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("file_offset");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("num_values");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("path_in_schema");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("type");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("stats_min");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("stats_max");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("stats_null_count");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("stats_distinct_count");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("stats_min_value");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("stats_max_value");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("compression");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("encodings");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("index_page_offset");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("dictionary_page_offset");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("data_page_offset");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("total_compressed_size");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("total_uncompressed_size");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("key_value_metadata");
	return_types.emplace_back(GetType());
}

Value ConvertParquetStats(const LogicalType &type, const duckdb_parquet::format::SchemaElement &schema_ele,
                          bool stats_is_set, const std::string &stats) {
	if (!stats_is_set) {
		return Value(LogicalType::VARCHAR);
	}
	return ParquetStatisticsUtils::ConvertValue(type, schema_ele, stats).DefaultCastAs(LogicalType::VARCHAR);
}

void ParquetMetaDataOperatorData::LoadFileMetaData(ClientContext &context, const vector<LogicalType> &return_types,
                                                   const string &file_path) {
	collection.Reset();
	ParquetOptions parquet_options(context);
	auto reader = make_uniq<ParquetReader>(context, file_path, parquet_options);
	idx_t count = 0;
	DataChunk current_chunk;
	current_chunk.Initialize(context, return_types);
	auto meta_data = reader->GetFileMetadata();
	vector<LogicalType> column_types;
	vector<idx_t> schema_indexes;
	for (idx_t schema_idx = 0; schema_idx < meta_data->schema.size(); schema_idx++) {
		auto &schema_element = meta_data->schema[schema_idx];
		if (schema_element.num_children > 0) {
			continue;
		}
		column_types.push_back(ParquetReader::DeriveLogicalType(schema_element, false));
		schema_indexes.push_back(schema_idx);
	}

	for (idx_t row_group_idx = 0; row_group_idx < meta_data->row_groups.size(); row_group_idx++) {
		auto &row_group = meta_data->row_groups[row_group_idx];

		if (row_group.columns.size() > column_types.size()) {
			throw InternalException("Too many column in row group: corrupt file?");
		}
		for (idx_t col_idx = 0; col_idx < row_group.columns.size(); col_idx++) {
			auto &column = row_group.columns[col_idx];
			auto &col_meta = column.meta_data;
			auto &stats = col_meta.statistics;
			auto &schema_element = meta_data->schema[schema_indexes[col_idx]];
			auto &column_type = column_types[col_idx];

			// file_name, LogicalType::VARCHAR
			current_chunk.SetValue(0, count, file_path);

			// row_group_id, LogicalType::BIGINT
			current_chunk.SetValue(1, count, Value::BIGINT(row_group_idx));

			// row_group_num_rows, LogicalType::BIGINT
			current_chunk.SetValue(2, count, Value::BIGINT(row_group.num_rows));

			// row_group_num_columns, LogicalType::BIGINT
			current_chunk.SetValue(3, count, Value::BIGINT(row_group.columns.size()));

			// row_group_bytes, LogicalType::BIGINT
			current_chunk.SetValue(4, count, Value::BIGINT(row_group.total_byte_size));

			// column_id, LogicalType::BIGINT
			current_chunk.SetValue(5, count, Value::BIGINT(col_idx));

			// file_offset, LogicalType::BIGINT
			current_chunk.SetValue(6, count, ParquetElementBigint(column.file_offset, row_group.__isset.file_offset));

			// num_values, LogicalType::BIGINT
			current_chunk.SetValue(7, count, Value::BIGINT(col_meta.num_values));

			// path_in_schema, LogicalType::VARCHAR
			current_chunk.SetValue(8, count, StringUtil::Join(col_meta.path_in_schema, ", "));

			// type, LogicalType::VARCHAR
			current_chunk.SetValue(9, count, ConvertParquetElementToString(col_meta.type));

			// stats_min, LogicalType::VARCHAR
			current_chunk.SetValue(10, count,
			                       ConvertParquetStats(column_type, schema_element, stats.__isset.min, stats.min));

			// stats_max, LogicalType::VARCHAR
			current_chunk.SetValue(11, count,
			                       ConvertParquetStats(column_type, schema_element, stats.__isset.max, stats.max));

			// stats_null_count, LogicalType::BIGINT
			current_chunk.SetValue(12, count, ParquetElementBigint(stats.null_count, stats.__isset.null_count));

			// stats_distinct_count, LogicalType::BIGINT
			current_chunk.SetValue(13, count, ParquetElementBigint(stats.distinct_count, stats.__isset.distinct_count));

			// stats_min_value, LogicalType::VARCHAR
			current_chunk.SetValue(
			    14, count, ConvertParquetStats(column_type, schema_element, stats.__isset.min_value, stats.min_value));

			// stats_max_value, LogicalType::VARCHAR
			current_chunk.SetValue(
			    15, count, ConvertParquetStats(column_type, schema_element, stats.__isset.max_value, stats.max_value));

			// compression, LogicalType::VARCHAR
			current_chunk.SetValue(16, count, ConvertParquetElementToString(col_meta.codec));

			// encodings, LogicalType::VARCHAR
			vector<string> encoding_string;
			encoding_string.reserve(col_meta.encodings.size());
			for (auto &encoding : col_meta.encodings) {
				encoding_string.push_back(ConvertParquetElementToString(encoding));
			}
			current_chunk.SetValue(17, count, Value(StringUtil::Join(encoding_string, ", ")));

			// index_page_offset, LogicalType::BIGINT
			current_chunk.SetValue(
			    18, count, ParquetElementBigint(col_meta.index_page_offset, col_meta.__isset.index_page_offset));

			// dictionary_page_offset, LogicalType::BIGINT
			current_chunk.SetValue(
			    19, count,
			    ParquetElementBigint(col_meta.dictionary_page_offset, col_meta.__isset.dictionary_page_offset));

			// data_page_offset, LogicalType::BIGINT
			current_chunk.SetValue(20, count, Value::BIGINT(col_meta.data_page_offset));

			// total_compressed_size, LogicalType::BIGINT
			current_chunk.SetValue(21, count, Value::BIGINT(col_meta.total_compressed_size));

			// total_uncompressed_size, LogicalType::BIGINT
			current_chunk.SetValue(22, count, Value::BIGINT(col_meta.total_uncompressed_size));

			current_chunk.SetValue(23, count, KeyValueToMap(col_meta.key_value_metadata));

			count++;
			if (count >= STANDARD_VECTOR_SIZE) {
				current_chunk.SetCardinality(count);
				collection.Append(current_chunk);

				count = 0;
				current_chunk.Reset();
			}
		}
	}
	current_chunk.SetCardinality(count);
	collection.Append(current_chunk);

	collection.InitializeScan(scan_state);
}
LogicalType ParquetMetaDataOperatorData::GetType() {
	return LogicalType::MAP(LogicalTypeId::VARCHAR, LogicalTypeId::VARCHAR);
}

void ParquetMetaDataOperatorData::BindSchema(vector<LogicalType> &return_types, vector<string> &names) {
	names.emplace_back("file_name");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("name");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("type");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("type_length");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("repetition_type");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("num_children");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("converted_type");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.emplace_back("scale");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("precision");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("field_id");
	return_types.emplace_back(LogicalType::BIGINT);

	names.emplace_back("logical_type");
	return_types.emplace_back(LogicalType::VARCHAR);
}

void ParquetMetaDataOperatorData::BindFileLevelMetaData(vector<LogicalType> &return_types, vector<string> &names) {
	names.push_back("file_name");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.push_back("column_orders");
	return_types.emplace_back(LogicalType::LIST(LogicalType::VARCHAR));

	names.push_back("key_value_metadata");
	return_types.emplace_back(GetType());

	names.push_back("created_by");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.push_back("encryption_algorithm");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.push_back("footer_signing_key_metadata");
	return_types.emplace_back(LogicalType::VARCHAR);

	names.push_back("num_rows");
	return_types.emplace_back(LogicalType::INTEGER);
}

Value ParquetLogicalTypeToString(const duckdb_parquet::format::LogicalType &type, bool is_set) {
	if (!is_set) {
		return Value();
	}
	if (type.__isset.STRING) {
		return Value(PrintParquetElementToString(type.STRING));
	}
	if (type.__isset.MAP) {
		return Value(PrintParquetElementToString(type.MAP));
	}
	if (type.__isset.LIST) {
		return Value(PrintParquetElementToString(type.LIST));
	}
	if (type.__isset.ENUM) {
		return Value(PrintParquetElementToString(type.ENUM));
	}
	if (type.__isset.DECIMAL) {
		return Value(PrintParquetElementToString(type.DECIMAL));
	}
	if (type.__isset.DATE) {
		return Value(PrintParquetElementToString(type.DATE));
	}
	if (type.__isset.TIME) {
		return Value(PrintParquetElementToString(type.TIME));
	}
	if (type.__isset.TIMESTAMP) {
		return Value(PrintParquetElementToString(type.TIMESTAMP));
	}
	if (type.__isset.INTEGER) {
		return Value(PrintParquetElementToString(type.INTEGER));
	}
	if (type.__isset.UNKNOWN) {
		return Value(PrintParquetElementToString(type.UNKNOWN));
	}
	if (type.__isset.JSON) {
		return Value(PrintParquetElementToString(type.JSON));
	}
	if (type.__isset.BSON) {
		return Value(PrintParquetElementToString(type.BSON));
	}
	if (type.__isset.UUID) {
		return Value(PrintParquetElementToString(type.UUID));
	}
	return Value();
}

void ParquetMetaDataOperatorData::LoadSchemaData(ClientContext &context, const vector<LogicalType> &return_types,
                                                 const string &file_path) {
	collection.Reset();
	ParquetOptions parquet_options(context);
	auto reader = make_uniq<ParquetReader>(context, file_path, parquet_options);
	idx_t count = 0;
	DataChunk current_chunk;
	current_chunk.Initialize(context, return_types);
	auto meta_data = reader->GetFileMetadata();
	for (idx_t col_idx = 0; col_idx < meta_data->schema.size(); col_idx++) {
		auto &column = meta_data->schema[col_idx];

		// file_name, LogicalType::VARCHAR
		current_chunk.SetValue(0, count, file_path);

		// name, LogicalType::VARCHAR
		current_chunk.SetValue(1, count, column.name);

		// type, LogicalType::VARCHAR
		current_chunk.SetValue(2, count, ParquetElementString(column.type, column.__isset.type));

		// type_length, LogicalType::INTEGER
		current_chunk.SetValue(3, count, ParquetElementInteger(column.type_length, column.__isset.type_length));

		// repetition_type, LogicalType::VARCHAR
		current_chunk.SetValue(4, count, ParquetElementString(column.repetition_type, column.__isset.repetition_type));

		// num_children, LogicalType::BIGINT
		current_chunk.SetValue(5, count, ParquetElementBigint(column.num_children, column.__isset.num_children));

		// converted_type, LogicalType::VARCHAR
		current_chunk.SetValue(6, count, ParquetElementString(column.converted_type, column.__isset.converted_type));

		// scale, LogicalType::BIGINT
		current_chunk.SetValue(7, count, ParquetElementBigint(column.scale, column.__isset.scale));

		// precision, LogicalType::BIGINT
		current_chunk.SetValue(8, count, ParquetElementBigint(column.precision, column.__isset.precision));

		// field_id, LogicalType::BIGINT
		current_chunk.SetValue(9, count, ParquetElementBigint(column.field_id, column.__isset.field_id));

		// logical_type, LogicalType::VARCHAR
		current_chunk.SetValue(10, count, ParquetLogicalTypeToString(column.logicalType, column.__isset.logicalType));

		count++;
		if (count >= STANDARD_VECTOR_SIZE) {
			current_chunk.SetCardinality(count);
			collection.Append(current_chunk);

			count = 0;
			current_chunk.Reset();
		}
	}
	current_chunk.SetCardinality(count);
	collection.Append(current_chunk);

	collection.InitializeScan(scan_state);
}
Value ParquetMetaDataOperatorData::KeyValueToMap(const vector<duckdb_parquet::format::KeyValue> &key_value_metadata) {
	auto values = vector<Value>();
	for (const auto &item : key_value_metadata) {
		values.emplace_back(Value::KEYVALUE(item.key, item.value));
	}
	return Value::MAP(ListType::GetChildType(GetType()), values);
}
void ParquetMetaDataOperatorData::LoadFileLevelMetadata(ClientContext &context, const vector<LogicalType> &return_types,
                                                        const string &file_path) {
	collection.Reset();
	ParquetOptions parquet_options(context);
	auto reader = make_uniq<ParquetReader>(context, file_path, parquet_options);
	idx_t count = 0;
	DataChunk current_chunk;
	current_chunk.Initialize(context, return_types);
	auto meta_data = reader->GetFileMetadata();

	// file_name, LogicalType::VARCHAR
	current_chunk.SetValue(0, count, file_path);

	// column_orders, LogicalType::LIST(LogicalType::VARCHAR)
	vector<Value> column_orders;
	for (auto const &item : meta_data->column_orders) {
		column_orders.emplace_back(PrintParquetElementToString(item));
	}
	current_chunk.SetValue(1, count, Value::LIST(column_orders));

	// key_value_metadata, LogicalType::MAP
	current_chunk.SetValue(2, count, KeyValueToMap(meta_data->key_value_metadata));

	// created_by, LogicalType::VARCHAR
	current_chunk.SetValue(3, count, meta_data->created_by);

	// encryption_algorithm, LogicalType::VARCHAR
	current_chunk.SetValue(4, count, PrintParquetElementToString(meta_data->encryption_algorithm));

	// footer_signing_key_metadata, LogicalType::VARCHAR
	current_chunk.SetValue(5, count, meta_data->footer_signing_key_metadata);

	// num_rows, LogicalType::INT64
	current_chunk.SetValue(6, count, meta_data->num_rows);

	count++;
	if (count >= STANDARD_VECTOR_SIZE) {
		current_chunk.SetCardinality(count);
		collection.Append(current_chunk);

		count = 0;
		current_chunk.Reset();
	}

	current_chunk.SetCardinality(count);
	collection.Append(current_chunk);

	collection.InitializeScan(scan_state);
}

template <int DATA_TYPE>
unique_ptr<FunctionData> ParquetMetaDataBind(ClientContext &context, TableFunctionBindInput &input,
                                             vector<LogicalType> &return_types, vector<string> &names) {
	if (DATA_TYPE == 1) {
		ParquetMetaDataOperatorData::BindSchema(return_types, names);
	} else if (DATA_TYPE == 0) {
		ParquetMetaDataOperatorData::BindMetaData(return_types, names);
	} else if (DATA_TYPE == 2) {
		ParquetMetaDataOperatorData::BindFileLevelMetaData(return_types, names);
	} else {
		D_ASSERT(0);
	}

	auto result = make_uniq<ParquetMetaDataBindData>();
	result->return_types = return_types;
	result->files = MultiFileReader::GetFileList(context, input.inputs[0], "Parquet");
	return std::move(result);
}

template <int DATA_TYPE>
unique_ptr<GlobalTableFunctionState> ParquetMetaDataInit(ClientContext &context, TableFunctionInitInput &input) {
	auto &bind_data = input.bind_data->Cast<ParquetMetaDataBindData>();
	D_ASSERT(!bind_data.files.empty());

	auto result = make_uniq<ParquetMetaDataOperatorData>(context, bind_data.return_types);
	if (DATA_TYPE == 1) {
		result->LoadSchemaData(context, bind_data.return_types, bind_data.files[0]);
	} else if (DATA_TYPE == 0) {
		result->LoadFileMetaData(context, bind_data.return_types, bind_data.files[0]);
	} else if (DATA_TYPE == 2) {
		result->LoadFileLevelMetadata(context, bind_data.return_types, bind_data.files[0]);
	} else {
		D_ASSERT(0);
	}
	result->file_index = 0;
	return std::move(result);
}

template <int DATA_TYPE>
void ParquetMetaDataImplementation(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = data_p.global_state->Cast<ParquetMetaDataOperatorData>();
	auto &bind_data = data_p.bind_data->Cast<ParquetMetaDataBindData>();

	while (true) {
		if (!data.collection.Scan(data.scan_state, output)) {
			if (data.file_index + 1 < bind_data.files.size()) {
				// load the metadata for the next file
				data.file_index++;
				ClientContext &context_1 = context;
				if (DATA_TYPE == 1) {
					data.LoadSchemaData(context_1, bind_data.return_types, bind_data.files[data.file_index]);
				} else if (DATA_TYPE == 0) {
					data.LoadFileMetaData(context_1, bind_data.return_types, bind_data.files[data.file_index]);
				} else if (DATA_TYPE == 2) {
					data.LoadFileLevelMetadata(context_1, bind_data.return_types, bind_data.files[data.file_index]);
				} else {
					D_ASSERT(0);
				}
				continue;
			} else {
				// no files remaining: done
				return;
			}
		}
		if (output.size() != 0) {
			return;
		}
	}
}

ParquetMetaDataFunction::ParquetMetaDataFunction()
    : TableFunction("parquet_metadata", {LogicalType::VARCHAR}, ParquetMetaDataImplementation<0>,
                    ParquetMetaDataBind<0>, ParquetMetaDataInit<0>) {
}

ParquetSchemaFunction::ParquetSchemaFunction()
    : TableFunction("parquet_schema", {LogicalType::VARCHAR}, ParquetMetaDataImplementation<1>, ParquetMetaDataBind<1>,
                    ParquetMetaDataInit<1>) {
}

ParquetFileMetadataFunction::ParquetFileMetadataFunction()
    : TableFunction("parquet_file_metadata", {LogicalType::VARCHAR}, ParquetMetaDataImplementation<2>,
                    ParquetMetaDataBind<2>, ParquetMetaDataInit<2>) {
}

} // namespace duckdb
