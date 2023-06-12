package org.duckdb.duckdb_jdbc;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import org.duckdb.duckdb_jdbc.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;

        String string;
        try {
            string = new JNIInterface().stringFromJNI();
        } catch (Throwable t) {
            string = t.toString();
        }

        tv.setText(string);
    }
}