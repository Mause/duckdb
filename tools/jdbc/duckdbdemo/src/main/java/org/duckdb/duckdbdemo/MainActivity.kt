package org.duckdb.duckdbdemo

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.widget.GridLayout
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import org.duckdb.DuckDBDriver
import org.duckdb.duckdbdemo.databinding.ActivityMainBinding
import java.lang.String.*
import java.sql.Array
import java.sql.Connection
import java.sql.ResultSet
import java.sql.SQLException
import java.util.Locale
import java.util.Objects
import java.util.concurrent.Executors
import java.util.function.Consumer

class MainActivity : AppCompatActivity() {
    private val pool = Executors.newCachedThreadPool()
    private lateinit var connect: Connection
    private lateinit var binding: ActivityMainBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val layoutInflater = this.layoutInflater
        binding = ActivityMainBinding.inflate(layoutInflater)
        binding.executeButton.setOnClickListener { onClick() }
        setContentView(binding.root)

        val duckDBDriver = DuckDBDriver()
        try {
            connect = duckDBDriver.connect("jdbc:duckdb:", null)
            title = duckDBVersion
            setGridData(getString(R.string.sql_input_helper_text))
        } catch (t: Throwable) {
            handleException(t)
        }
    }

    @get:Throws(SQLException::class)
    private val duckDBVersion: String
        get() {
            connect.prepareStatement("select concat('DuckDB - ', version())").use { stmt ->
                stmt.executeQuery().use { resultSet ->
                    resultSet.next()
                    return resultSet.getString(1)
                }
            }
        }

    private fun onClick() {
        val editText = binding.sqlInput.editText
        pool.submit {
            Looper.prepare()
            if (editText != null) {
                setGridData(editText.text.toString())
            }
        }
    }

    private fun setGridData(sql: String) {
        val results = binding.results
        try {
            connect.prepareStatement(sql).use { stmt ->
                if (stmt.execute()) {
                    stmt.resultSet.use { resultSet -> loadIntoGridView(results, resultSet) }
                } else {
                    Toast.makeText(this, String.format(Locale.getDefault(), "Updated %d rows", stmt.updateCount), Toast.LENGTH_SHORT).show()
                }
            }
        } catch (t: Throwable) {
            handleException(t)
        }
    }

    private fun handleException(toHandle: Throwable) {
        var t: Throwable? = toHandle
        while (t!!.cause is SQLException) {
            t = t.cause
        }
        t.printStackTrace()
        var message = t.message
        if (message != null && message.contains(": ")) {
            val parts = message.split(": ".toRegex(), limit = 2).toTypedArray()
            message = parts[1]
        }
        Toast.makeText(this, message, Toast.LENGTH_LONG).show()
    }

    @Throws(SQLException::class)
    private fun loadIntoGridView(results: GridLayout, resultSet: ResultSet) {
        val metaData = resultSet.metaData
        val columnCount = metaData.columnCount
        val views = ArrayList<String>()
        var rows = 1
        for (i in 0 until columnCount) {
            views.add(metaData.getColumnName(i + 1))
        }
        while (resultSet.next()) {
            for (i in 0 until columnCount) {
                val obj = resultSet.getObject(i + 1)
                val string = if (obj is Array) join(", ", Util.sqlArrayToList<String>(obj)) else Objects.toString(obj)
                views.add(string)
            }
            rows++
        }
        val finalRows = rows
        Handler(Looper.getMainLooper()).post {
            results.removeAllViews()
            results.columnCount = columnCount
            views.forEach(Consumer { s: String -> results.addView(make(s)) })
            results.rowCount = finalRows
        }
    }

    private fun make(string: String): TextView {
        val child = TextView(this)
        val padding = 10
        child.setPadding(padding, padding, padding, padding)
        child.text = string
        return child
    }
}