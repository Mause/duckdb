package org.duckdb.duckdb_jdbc;

import android.os.Bundle;
import android.widget.GridLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import org.duckdb.DuckDBDriver;
import org.duckdb.duckdb_jdbc.databinding.ActivityMainBinding;

import java.sql.Array;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.Objects;

public class MainActivity extends AppCompatActivity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		var layoutInflater = getLayoutInflater();
		var binding = ActivityMainBinding.inflate(layoutInflater);
		setContentView(binding.getRoot());

		var results = binding.results;

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

				loadIntoGridView(results, resultSet);
			}
		} catch (Throwable t) {
			t.printStackTrace();
			Toast.makeText(this, t.toString(), Toast.LENGTH_LONG).show();
		}
	}

	private void loadIntoGridView(GridLayout results, ResultSet resultSet) throws SQLException {
		var metaData = resultSet.getMetaData();
		results.setColumnCount(metaData.getColumnCount());

		int rows = 1;
		for (int i = 0; i< metaData.getColumnCount(); i++) {
			results.addView(make(metaData.getColumnName(i+1)));
		}

		while (resultSet.next()) {
			for (int i = 0; i< metaData.getColumnCount(); i++) {
				Object object = resultSet.getObject(i + 1);
				String string;
				if (object instanceof Array) {
					string = String.join(", ", Util.<String>sqlArrayToList((Array) object));
				} else {
					string = Objects.toString(object);
				}
				results.addView(make(string));
			}
			rows++;
		}
		results.setRowCount(rows);
	}

	@NonNull
	private TextView make(String string) {
		TextView child = new TextView(this);
		child.setPadding(10, 0, 0, 0);
		child.setText(string);
		return child;
	}
}
