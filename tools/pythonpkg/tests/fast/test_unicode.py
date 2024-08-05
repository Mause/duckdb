#!/usr/bin/env python
# -*- coding: utf-8 -*-

import duckdb
from pytest import importorskip

pd = importorskip('pandas')


class TestUnicode(object):
    def test_unicode_pandas_scan(self, duckdb_cursor):
        con = duckdb.connect(database=':memory:', read_only=False)
        test_df = pd.DataFrame.from_dict({"i": [1, 2, 3], "j": ["a", "c", u"ë"]})
        con.register('test_df_view', test_df)
        con.execute('SELECT i, j, LENGTH(j) FROM test_df_view').fetchall()
