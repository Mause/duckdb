package org.duckdb.exceptions;

public class OperationalError extends Error {
    public OperationalError(String message) {
        super(message);
    }
}
