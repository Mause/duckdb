package org.duckdb.exceptions;

public class TransactionException extends OperationalError {
    public TransactionException(String message) {
        super(message);
    }
}
