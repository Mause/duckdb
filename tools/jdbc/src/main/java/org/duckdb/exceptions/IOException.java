package org.duckdb.exceptions;

public class IOException extends OperationalError {
    public IOException(String msg) {
        super(msg);
    }
}
