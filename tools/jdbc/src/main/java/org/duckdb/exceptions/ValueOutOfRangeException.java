package org.duckdb.exceptions;

public class ValueOutOfRangeException extends DataError {
    public ValueOutOfRangeException(String message) {
        super(message);
    }
}
