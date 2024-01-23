package org.duckdb;

import java.sql.SQLException;
import java.util.HashMap;
import java.util.Map;

final class JdbcUtils {

    @SuppressWarnings("unchecked")
    static <T> T unwrap(Object obj, Class<T> iface) throws SQLException {
        if (!iface.isInstance(obj)) {
            throw new SQLException(obj.getClass().getName() + " not unwrappable from " + iface.getName());
        }
        return (T) obj;
    }

    static <K, V> Map<K, V> mapOf(Object... pairs) {
        Map<K, V> result = new HashMap<>(pairs.length / 2);
        for (int i = 0; i < pairs.length - 1; i += 2) {
            result.put((K) pairs[i], (V) pairs[i + 1]);
        }
        return result;
    }

    private JdbcUtils() {
    }
}
