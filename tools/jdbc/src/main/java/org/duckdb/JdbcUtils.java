package org.duckdb;

import java.sql.SQLException;
import java.util.HashMap;
import java.util.Map;
import java.io.InputStream;
import java.io.IOException;
import java.io.ByteArrayOutputStream;

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

    static byte[] readAllBytes(InputStream x) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] thing = new byte[256];
        int length;
        int offset = 0;
        while ((length = x.read(thing)) != -1) {
            out.write(thing, offset, length);
            offset += length;
        }
        return out.toByteArray();
    }

    private JdbcUtils() {
    }
}
