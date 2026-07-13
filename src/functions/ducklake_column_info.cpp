#include "functions/ducklake_table_functions.hpp"
#include "storage/ducklake_catalog.hpp"
#include "storage/ducklake_schema_entry.hpp"
#include "storage/ducklake_table_entry.hpp"
#include "storage/ducklake_transaction.hpp"

namespace duckdb {

static unique_ptr<FunctionData> DuckLakeColumnInfoBind(ClientContext &context, TableFunctionBindInput &input,
                                                       vector<LogicalType> &return_types, vector<string> &names) {
	auto &catalog = DuckLakeBaseMetadataFunction::GetCatalog(context, input.inputs[0]).Cast<DuckLakeCatalog>();
	auto &transaction = DuckLakeTransaction::Get(context, catalog);
	auto result = make_uniq<MetadataBindData>();

	auto &schemas = catalog.GetSchemaForSnapshot(transaction, transaction.GetSnapshot());
	for (auto &schema_entry : schemas.GetEntries()) {
		auto &schema = schema_entry.second->Cast<DuckLakeSchemaEntry>();
		schema.Scan(CatalogType::TABLE_ENTRY, [&](const CatalogEntry &catalog_entry) {
			if (catalog_entry.type != CatalogType::TABLE_ENTRY) {
				return;
			}
			auto &table = catalog_entry.Cast<DuckLakeTableEntry>();
			auto table_info = table.GetTableInfo();
			auto columns = table.GetTableColumns();
			for (auto &column : columns) {
				vector<Value> row_values;
				row_values.push_back(Value(schema.name.GetIdentifierName()));
				row_values.push_back(Value::BIGINT(NumericCast<int64_t>(schema.GetSchemaId().index)));
				row_values.push_back(Value::UUID(schema.GetSchemaUUID()));
				row_values.push_back(Value(table_info.name));
				row_values.push_back(Value::BIGINT(NumericCast<int64_t>(table_info.id.index)));
				row_values.push_back(Value::UUID(table_info.uuid));
				row_values.push_back(Value(column.name));
				row_values.push_back(Value::BIGINT(NumericCast<int64_t>(column.id.index)));
				result->rows.push_back(std::move(row_values));
			}
		});
	}

	names = {"schema_name", "schema_id",  "schema_uuid", "table_name",
	         "table_id",    "table_uuid", "column_name", "column_id"};
	return_types = {LogicalType::VARCHAR, LogicalType::BIGINT, LogicalType::UUID,    LogicalType::VARCHAR,
	                LogicalType::BIGINT,  LogicalType::UUID,   LogicalType::VARCHAR, LogicalType::BIGINT};
	return std::move(result);
}

DuckLakeColumnInfoFunction::DuckLakeColumnInfoFunction()
    : DuckLakeBaseMetadataFunction("ducklake_column_info", DuckLakeColumnInfoBind) {
}

} // namespace duckdb
