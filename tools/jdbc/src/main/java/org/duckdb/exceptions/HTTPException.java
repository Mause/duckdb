package org.duckdb.exceptions;

import java.util.Map;

class HTTPException extends IOException {
    int statusCode;
    String body;
    String reason;
    Map<String, String> headers;

    public HTTPException(String msg, int statusCode, String body, String reason, Map<String, String> headers) {
        super(msg);
        this.statusCode = statusCode;
        this.body = body;
        this.reason = reason;
        this.headers = headers;
    }
}
