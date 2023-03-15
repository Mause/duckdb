package org.duckdb.exceptions;

public class InvalidInputException extends ProgrammingError {
    public InvalidInputException(String message) {
        super(message);
    }
}
