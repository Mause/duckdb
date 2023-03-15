package org.duckdb.exceptions;

public class SerializationException extends OperationalError {
    public SerializationException(String message) {
        super(message);
    }
}
