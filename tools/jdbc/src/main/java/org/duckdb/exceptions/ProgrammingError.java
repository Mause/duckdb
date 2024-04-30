package org.duckdb.exceptions;

public class ProgrammingError extends Error {
    public ProgrammingError(String message) {
        super(message);
    }
}
