package org.duckdb.exceptions;

public class ConnectionException extends OperationalError {
    public ConnectionException(String message) {
        super(message);
    }
}
