package org.duckdb.exceptions;

public class SyntaxException extends ProgrammingError {
    public SyntaxException(String message) {
        super(message);
    }
}
