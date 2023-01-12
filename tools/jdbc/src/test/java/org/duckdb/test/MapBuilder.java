package org.duckdb.test;

import java.util.HashMap;
import java.util.Map;

class MapBuilder<K, V> {
    private final Map<K, V> backer = new HashMap<>();

    MapBuilder<K, V> put(K key, V value) {
        backer.put(key, value);
        return this;
    }

    Map<K, V> build() {
        return new HashMap<>(backer);
    }
}
