# cursor description
from datetime import datetime, date

import duckdb
from pytest import mark, fixture


@fixture()
def conn():
    with duckdb.connect() as c:
        yield c


class TestCursorDescription(object):
    @mark.parametrize(
        "query,column_name,string_type,real_type",
        [
            ["SELECT * FROM integers", "i", "NUMBER", int],
            ["SELECT * FROM timestamps", "t", "DATETIME", datetime],
            ["SELECT DATE '1992-09-20' AS date_col;", "date_col", "Date", date],
            ["SELECT '\\xAA'::BLOB AS blob_col;", "blob_col", "BINARY", bytes],
            ["SELECT {'x': 1, 'y': 2, 'z': 3} AS struct_col", "struct_col", "dict", dict],
            ["SELECT [1, 2, 3] AS list_col", "list_col", "list", list],
            ["SELECT 'Frank' AS str_col", "str_col", "STRING", str],
            ["SELECT [1, 2, 3]::JSON AS json_col", "json_col", "STRING", str],
            ["SELECT union_value(tag := 1) AS union_col", "union_col", "UNION(tag INTEGER)", int],
        ],
    )
    def test_description(self, query, column_name, string_type, real_type, duckdb_cursor, timestamps, integers):
        duckdb_cursor.execute(query)
        assert duckdb_cursor.description == [(column_name, string_type, None, None, None, None, None)]
        assert isinstance(duckdb_cursor.fetchone()[0], real_type)

    def test_none_description(self, duckdb_empty_cursor):
        assert duckdb_empty_cursor.description is None

    def test_rowcount(self):
        conn = duckdb.connect()
        ex = conn.execute
        assert ex('select 1').rowcount == 1

        assert ex('create table test (id int)').rowcount == -1  # does not update or return rows

        assert ex('insert into test values (1)').rowcount == 1
        assert ex('update test set id = 2').rowcount == 1
        assert ex('update test set id = 2 where id = 1').rowcount == 0  # no matched rows, so no updates

    def test_rowcount_1(self, conn):
        # When running after fetchall has been run on a result
        conn.execute('select 1')
        conn.fetchall()
        assert conn.rowcount == 1

    def test_rowcount_2(self, conn):
        # When execute statements have been issued with multiple queries inside of them
        assert conn.execute('select 1; select 2').rowcount == 1

    def test_rowcount_3(self, conn):
        ex = conn.execute

        assert ex('create table test (id int)').rowcount == -1  # does not update or return rows

        # When an execute that returns a rowcount has been issued, followed by execute without a rowcount (e.g. INSERT followed by SELECT, followed by then fetching the rowcount)
        ex('insert into test values (1)')
        assert ex('select 1').rowcount == 1

        assert ex('set threads to 10').rowcount == -1


class TestCursorRowcount(object):
    def test_rowcount(self, duckdb_cursor):
        assert duckdb_cursor.rowcount == -1
