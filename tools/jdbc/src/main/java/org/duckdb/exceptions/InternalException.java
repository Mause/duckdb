package org.duckdb.exceptions;

public class InternalException extends InternalError {
    public InternalException(String message) {
        super(message);
    }
}
