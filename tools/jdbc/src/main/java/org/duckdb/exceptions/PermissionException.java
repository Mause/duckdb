package org.duckdb.exceptions;

public class PermissionException extends Error {
    public PermissionException(String message) {
        super(message);
    }
}
