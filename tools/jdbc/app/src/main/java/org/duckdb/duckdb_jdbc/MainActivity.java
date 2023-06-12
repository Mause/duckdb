package org.duckdb.duckdb_jdbc;

import android.os.Bundle;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import org.duckdb.DuckDBDriver;
import org.duckdb.duckdb_jdbc.databinding.ActivityMainBinding;

import java.sql.SQLException;

public class MainActivity extends AppCompatActivity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		var layoutInflater = getLayoutInflater();
		var binding = ActivityMainBinding.inflate(layoutInflater);
		setContentView(binding.getRoot());

		var adapter = new ExtensionArrayAdapter(this);
		binding.results.setAdapter(adapter);

		var duckDBDriver = new DuckDBDriver();
		try (var connect = duckDBDriver.connect("jdbc:duckdb:", null)) {
			try (var stmt = connect.prepareStatement("select concat('We are running DuckDB version ', version())");
				 var resultSet = stmt.executeQuery()) {
				if (resultSet.next()) {
					binding.sampleText.setText(resultSet.getString(1));
				}
			}

			try (var stmt = connect.prepareStatement("select * from duckdb_extensions()");
				 var resultSet = stmt.executeQuery()) {
				while (resultSet.next()) {
					adapter.add(new Extension(resultSet));
				}
			}
		} catch (SQLException e) {
			Toast.makeText(this, e.toString(), Toast.LENGTH_LONG).show();
		}
	}
}
