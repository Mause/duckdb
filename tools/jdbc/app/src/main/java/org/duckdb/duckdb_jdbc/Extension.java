package org.duckdb.duckdb_jdbc;

import java.sql.ResultSet;
import java.sql.SQLException;

class Extension {
	String name;
	boolean installed;

	public Extension(ResultSet resultSet) throws SQLException {
		name = resultSet.getString("extension_name");
		installed = resultSet.getBoolean("installed");
	}
}
