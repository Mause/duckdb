package org.duckdb.exceptions;

public class InvalidTypeException extends ProgrammingError {
    public InvalidTypeException(String message) {
        super(message);
    }
}
