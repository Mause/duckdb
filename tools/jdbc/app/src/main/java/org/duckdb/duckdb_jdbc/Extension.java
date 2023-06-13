package org.duckdb.duckdb_jdbc;

import java.util.Arrays;
import java.util.List;
import java.sql.ResultSet;
import java.sql.SQLException;

class Extension {
	String name;
	boolean installed;
	boolean loaded;
	String install_path;
	String description;
	List<String> aliases;

	public Extension(ResultSet resultSet) throws SQLException {
		name = resultSet.getString("extension_name");
		installed = resultSet.getBoolean("installed");
		loaded = resultSet.getBoolean("loaded");
		install_path = resultSet.getString("install_path");
        description = resultSet.getString("description");
        aliases = (List<String>) Arrays.asList(resultSet.getArray("aliases").getArray());
	}
}
