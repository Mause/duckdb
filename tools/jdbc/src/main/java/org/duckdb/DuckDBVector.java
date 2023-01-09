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

	protected ByteBuffer getbuf(int chunk_idx, int typeWidth) {
		ByteBuffer buf = constlen_data;
		buf.order(ByteOrder.LITTLE_ENDIAN);
		buf.position((chunk_idx - 1) * typeWidth);
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
	public Object[] toArray() { return varlen_data; }
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
