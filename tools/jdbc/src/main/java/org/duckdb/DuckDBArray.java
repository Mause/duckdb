package org.duckdb;

import java.sql.Array;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.Connection;
import java.sql.Date;
import java.sql.NClob;
import java.sql.ParameterMetaData;
import java.sql.PreparedStatement;
import java.sql.Ref;
import java.sql.ResultSet;
import java.util.List;
import java.util.Map;

public class DuckDBArray implements java.sql.Array {
	private final List<Object> array;

	public DuckDBArray(List<Object> array) { this.array = array; }

	@Override
	public void free() {
		throw new UnsupportedOperationException();
	}
	/**
	 * Retrieves the JDBC type of the elements in the array designated by this
	 * Array object.
	 */
	@Override
	public int getBaseType() {
		throw new UnsupportedOperationException();
	}
	/**
	 * Retrieves the SQL type name of the elements in the array designated by
	 * this Array object.
	 */
	@Override
	public String getBaseTypeName() {
		throw new UnsupportedOperationException();
	}
	@Override
	public ResultSet getResultSet(Map<String, Class<?>> typeMap) {
		throw new UnsupportedOperationException();
	}
	@Override
	public ResultSet getResultSet() {
		throw new UnsupportedOperationException();
	}
	@Override
	public ResultSet getResultSet(long index, int count) {
		throw new UnsupportedOperationException();
	}
	@Override
	public Object getArray() {
		return array.toArray();
	}
	@Override
	public Object getArray(long index, int count) {
		throw new UnsupportedOperationException();
	}
	@Override
	public Object getArray(Map<String, Class<?>> typeMap) {
		throw new UnsupportedOperationException();
	}
	@Override
	public Object getArray(long index, int count,
	                       Map<String, Class<?>> typeMap) {
		throw new UnsupportedOperationException();
	}
	@Override
	public ResultSet getResultSet(long index, int count,
	                              Map<String, Class<?>> typeMap) {
		throw new UnsupportedOperationException();
	}
}