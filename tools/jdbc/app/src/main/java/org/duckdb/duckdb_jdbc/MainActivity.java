package org.duckdb.duckdb_jdbc;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import org.duckdb.duckdb_jdbc.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'duckdb_jdbc' library on application startup.
    static {
        System.loadLibrary("duckdb_jdbc");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
    }

    /**
     * A native method that is implemented by the 'duckdb_jdbc' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}