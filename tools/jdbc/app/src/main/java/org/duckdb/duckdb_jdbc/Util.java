package org.duckdb.duckdb_jdbc;

import androidx.annotation.NonNull;

import java.sql.Array;
import java.sql.SQLException;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

public class Util {
	@NonNull
	@SuppressWarnings("unchecked")
	public static <T> List<T> sqlArrayToList(Array array) throws SQLException {
		return Arrays.stream((T[]) array.getArray()).collect(Collectors.toList());
	}
}
