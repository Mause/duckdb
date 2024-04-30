package org.duckdb.exceptions;

public class TypeMismatchException extends DataError {
    public TypeMismatchException(String message) {
        super(message);
    }
}
