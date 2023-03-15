package org.duckdb.exceptions;

public class InternalError extends Error {
    public InternalError(String message) {
        super(message);
    }
}
