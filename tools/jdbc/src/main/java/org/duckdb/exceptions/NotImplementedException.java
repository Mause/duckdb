package org.duckdb.exceptions;

public class NotImplementedException extends NotSupportedError {
    public NotImplementedException(String message) {
        super(message);
    }
}
