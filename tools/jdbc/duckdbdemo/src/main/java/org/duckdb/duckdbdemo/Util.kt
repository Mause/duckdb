package org.duckdb.duckdbdemo

import java.util.Arrays
import java.util.stream.Collectors

object Util {
    @SuppressWarnings("unchecked")
    fun <T> sqlArrayToList(array: java.sql.Array): List<T> {
        return Arrays.stream(array.array as Array<T>).collect(Collectors.toList())
    }
}