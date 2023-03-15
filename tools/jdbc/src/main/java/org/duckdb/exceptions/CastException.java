package org.duckdb.exceptions;

public class CastException extends DataError {
    public CastException(String message) {
        super(message);
    }
}
