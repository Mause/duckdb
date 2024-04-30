package org.duckdb.exceptions;

public class FatalException extends Error {
    public FatalException(String message) {
        super(message);
    }
}
