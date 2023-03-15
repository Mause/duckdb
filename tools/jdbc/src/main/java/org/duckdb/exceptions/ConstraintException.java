package org.duckdb.exceptions;

public class ConstraintException extends IntegrityError {
    public ConstraintException(String message) {
        super(message);
    }
}
