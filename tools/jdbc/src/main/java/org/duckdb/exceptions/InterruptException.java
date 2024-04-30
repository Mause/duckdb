package org.duckdb.exceptions;

public class InterruptException extends Error {
    public InterruptException(String message) {
        super(message);
    }
}
