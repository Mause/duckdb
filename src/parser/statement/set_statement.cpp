#include "duckdb/parser/statement/set_statement.hpp"
#include "duckdb/common/serializer/format_serializer.hpp"

namespace duckdb {

SetStatement::SetStatement(std::string name_p, SetScope scope_p, SetType type_p)
    : SQLStatement(StatementType::SET_STATEMENT), name(std::move(name_p)), scope(scope_p), set_type(type_p) {
}

unique_ptr<SQLStatement> SetStatement::Copy() const {
	return unique_ptr<SetStatement>(new SetStatement(*this));
}

void SetStatement::FormatSerialize(FormatSerializer &serializer) const {
	SQLStatement::FormatSerialize(serializer);

	serializer.WriteProperty("class", "SET_STATEMENT");
	serializer.WriteProperty("name", name);
}

// Set Variable

SetVariableStatement::SetVariableStatement(std::string name_p, Value value_p, SetScope scope_p)
    : SetStatement(std::move(name_p), scope_p, SetType::SET), value(std::move(value_p)) {
}

unique_ptr<SQLStatement> SetVariableStatement::Copy() const {
	return unique_ptr<SetVariableStatement>(new SetVariableStatement(*this));
}

void SetVariableStatement::FormatSerialize(FormatSerializer &serializer) const {
	SetStatement::FormatSerialize(serializer);
	serializer.WriteProperty("type", "VARIABLE");
	serializer.WriteProperty("value", value);
}

// Reset Variable

ResetVariableStatement::ResetVariableStatement(std::string name_p, SetScope scope_p)
    : SetStatement(std::move(name_p), scope_p, SetType::RESET) {
}

void ResetVariableStatement::FormatSerialize(FormatSerializer &serializer) const {
	SetStatement::FormatSerialize(serializer);
	serializer.WriteProperty("type", "RESET");
}

} // namespace duckdb
