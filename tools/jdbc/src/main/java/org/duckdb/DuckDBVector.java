package org.duckdb;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Collection;
import java.util.Iterator;

public class DuckDBVector<T> implements Collection<T> {

	public DuckDBVector(String duckdb_type, int length, boolean[] nullmask) {
		super();
		this.duckdb_type = duckdb_type;
		this.length = length;
		this.nullmask = nullmask;
	}
	protected String duckdb_type;
	protected int length;
	protected boolean[] nullmask;
	protected ByteBuffer constlen_data = null;
	protected Object[] varlen_data = null;

	protected ByteBuffer getbuf(int columnIndex, int typeWidth) {
		ByteBuffer buf = constlen_data;
		buf.order(ByteOrder.LITTLE_ENDIAN);
		buf.position(columnIndex * typeWidth);
		return buf;
	}

	public void clear() { throw new UnsupportedOperationException(); }
	public boolean retainAll(Collection<?> coll) {
		throw new UnsupportedOperationException();
	}
	public boolean removeAll(Collection<?> coll) {
		throw new UnsupportedOperationException();
	}
	public boolean addAll(Collection<? extends T> coll) {
		throw new UnsupportedOperationException();
	}
	public boolean containsAll(Collection<?> coll) {
		throw new UnsupportedOperationException();
	}
	public boolean remove(Object o) {
		throw new UnsupportedOperationException();
	}

	public Object[] toArray() {

		Object[] data = new Object[length];
		for (int i = 0; i < length; i++) {
			data[i] = getObject(i);
		}

		return data;
	}
	public <T> T[] toArray(T[] o) { throw new UnsupportedOperationException(); }
	public boolean add(T o) { throw new UnsupportedOperationException(); }
	public boolean contains(Object o) {
		throw new UnsupportedOperationException();
	}
	public boolean isEmpty() { return length == 0; }
	public int size() { return length; }
	public Iterator<T> iterator() {
		return new Iterator() {
			private int index = 0;
			public boolean hasNext() { return length < index; }
			public T next() { return (T)varlen_data[index++]; }
		};
	}

	public Object getObject(int i) {
		DuckDBColumnType type = DuckDBResultSetMetaData.TypeNameToType(duckdb_type);
		switch (type) {
			case INTEGER:
				return getInt(i);
			case VARCHAR:
				return getLazyString(i);
			case UTINYINT:
				return getShort(i);
			default:
				throw new IllegalStateException(String.format("unsupported list type: %s", duckdb_type));
		}
	}

	public String getLazyString(int columnIndex) {
		return (String) varlen_data[columnIndex];
	}

	public int getInt(int columnIndex) {
		return getbuf(columnIndex, 4).getInt();
	}

	public short getShort(int columnIndex) {
		return getbuf(columnIndex, 2).getShort();
	}
}
