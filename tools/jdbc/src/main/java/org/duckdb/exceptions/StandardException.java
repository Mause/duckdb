package org.duckdb.exceptions;

import java.sql.SQLException;

public class StandardException extends Error {
     public StandardException(String message) {
         super(message);
     }
 }

