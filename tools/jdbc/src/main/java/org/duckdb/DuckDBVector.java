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

	private void precondition(boolean condition, String message) {
		if (!condition) {
			throw new IllegalStateException(message);
		}
	}
	private int getInt(int columnIndex) {
		return getbuf(columnIndex, 4).getInt();
	}
	public Object[] toArray() {
		precondition(duckdb_type.equals("INTEGER"), "only supportng integers for now");

		Object[] data = new Object[length];
		for (int j=0; j<length; j++) {
			data[j] = getInt(j);			
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
}
