package org.duckdb.exceptions;

public class NotSupportedError extends Error {
    public NotSupportedError(String message) {
        super(message);
    }
}
