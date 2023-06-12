package org.duckdb.duckdb_jdbc;

import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;

import org.duckdb.duckdb_jdbc.databinding.ResultItemBinding;

class ExtensionArrayAdapter extends ArrayAdapter<Extension> {
	private final MainActivity mainActivity;

	public ExtensionArrayAdapter(MainActivity mainActivity) {
		super(mainActivity, R.layout.result_item, R.id.result_item);
		this.mainActivity = mainActivity;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		var item = getItem(position);
		ResultItemBinding resultItemBinding;
		if (convertView == null) {
			resultItemBinding = ResultItemBinding.inflate(mainActivity.getLayoutInflater(), parent, false);
		} else {
			resultItemBinding = ResultItemBinding.bind(convertView);
		}
		resultItemBinding.resultItemName.setText(item.name);
		resultItemBinding.resultItemInstalled.setText(String.valueOf(item.installed));
		return resultItemBinding.getRoot();
	}
}
