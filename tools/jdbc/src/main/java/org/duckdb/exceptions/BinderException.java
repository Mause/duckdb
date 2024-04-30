package org.duckdb.exceptions;

public class BinderException extends ProgrammingError {
    public BinderException(String message) {
        super(message);
    }
}
