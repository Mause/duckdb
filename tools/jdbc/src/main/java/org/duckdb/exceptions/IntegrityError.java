package org.duckdb.exceptions;

public class IntegrityError extends Error {
    public IntegrityError(String message) {
        super(message);
    }
}
