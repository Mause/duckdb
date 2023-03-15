package org.duckdb.exceptions;

public class OutOfRangeException extends DataError {
    public OutOfRangeException(String message) {
        super(message);
    }
}
