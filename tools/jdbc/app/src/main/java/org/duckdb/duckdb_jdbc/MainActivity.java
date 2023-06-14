package org.duckdb.duckdb_jdbc;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.EditText;
import android.widget.GridLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import org.duckdb.DuckDBDriver;
import org.duckdb.duckdb_jdbc.databinding.ActivityMainBinding;

import java.sql.Array;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.Locale;
import java.util.Objects;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends AppCompatActivity {
	private final ExecutorService pool = Executors.newCachedThreadPool();
	private Connection connect;
	private ActivityMainBinding binding;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		var layoutInflater = getLayoutInflater();
		binding = ActivityMainBinding.inflate(layoutInflater);
		binding.executeButton.setOnClickListener(this::onClick);
		setContentView(binding.getRoot());

		var duckDBDriver = new DuckDBDriver();
		try {
			connect = duckDBDriver.connect("jdbc:duckdb:", null);

			setTitle(getDuckDBVersion());

			setGridData(getString(R.string.sql_input_helper_text));
		} catch (Throwable t) {
			handleException(t);
		}
	}

	private String getDuckDBVersion() throws SQLException {
		try (var stmt = connect.prepareStatement("select concat('DuckDB - ', version())");
			 var resultSet = stmt.executeQuery()) {
			resultSet.next();
			return resultSet.getString(1);
		}
	}

	private void onClick(View view) {
		EditText editText = binding.sqlInput.getEditText();
		if (editText != null) {
			pool.submit(() -> {
				Looper.prepare();
				setGridData(editText.getText().toString());
			});
		}
	}

	private void setGridData(String sql) {
		var results = binding.results;
		try (var stmt = connect.prepareStatement(sql)) {
			if(stmt.execute()) {
				try (var resultSet = stmt.getResultSet()) {
					loadIntoGridView(results, resultSet);
				}
			} else {
				Toast.makeText(this, String.format(Locale.getDefault(), "Updated %d rows", stmt.getUpdateCount()), Toast.LENGTH_SHORT).show();
			}
		} catch (Throwable t) {
			handleException(t);
		}
	}

	private void handleException(Throwable t) {
		while (t.getCause() instanceof SQLException) {
			t = t.getCause();
		}
		t.printStackTrace();
		var message = t.getMessage();
		if (message != null && message.contains(": ")) {
			String[] parts = message.split(": ", 2);
			message = parts[1];
		}
		Toast.makeText(this, message, Toast.LENGTH_LONG).show();
	}

	private void loadIntoGridView(GridLayout results, ResultSet resultSet) throws SQLException {
		var metaData = resultSet.getMetaData();
		int columnCount = metaData.getColumnCount();

		var views = new ArrayList<String>();

		int rows = 1;
		for (int i = 0; i < columnCount; i++) {
			views.add(metaData.getColumnName(i + 1));
		}

		while (resultSet.next()) {
			for (int i = 0; i < columnCount; i++) {
				Object object = resultSet.getObject(i + 1);
				String string;
				if (object instanceof Array) {
					string = String.join(", ", Util.<String>sqlArrayToList((Array) object));
				} else {
					string = Objects.toString(object);
				}
				views.add(string);
			}
			rows++;
		}

		int finalRows = rows;
		new Handler(Looper.getMainLooper()).post(() -> {
			results.removeAllViews();
			results.setColumnCount(columnCount);
			views.forEach(s -> results.addView(make(s)));
			results.setRowCount(finalRows);
		});
	}

	@NonNull
	private TextView make(String string) {
		TextView child = new TextView(this);
		int padding = 10;
		child.setPadding(padding, padding, padding, padding);
		child.setText(string);
		return child;
	}
}
