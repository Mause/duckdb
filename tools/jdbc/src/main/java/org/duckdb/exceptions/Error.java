package org.duckdb.exceptions;

import java.sql.SQLException;

public class Error extends SQLException {
    public Error(String message) {
        super(message);
    }
}
