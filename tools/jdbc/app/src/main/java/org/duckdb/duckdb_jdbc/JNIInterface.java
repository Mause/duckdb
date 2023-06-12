package org.duckdb.duckdb_jdbc;

public class JNIInterface {

    // Used to load the 'duckdb_jdbc' library on application startup.
    static {
        System.loadLibrary("duckdb_java");
    }

    /**
     * A native method that is implemented by the 'duckdb_jdbc' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}
