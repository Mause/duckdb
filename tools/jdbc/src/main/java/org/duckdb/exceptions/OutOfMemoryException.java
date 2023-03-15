package org.duckdb.exceptions;

public class OutOfMemoryException extends OperationalError {
    public OutOfMemoryException(String message) {
        super(message);
    }
}
