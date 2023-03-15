package org.duckdb.exceptions;

public class CatalogException extends ProgrammingError {
    public CatalogException(String message) {
        super(message);
    }
}
