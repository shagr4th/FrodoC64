package org.ab.c64;

import java.io.File;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import org.ab.nativelayer.ImportFileView;

import android.app.Application;

public class C64ImportView extends ImportFileView {

	private static final long serialVersionUID = 154402610131021644L;
	private Map<File, ArrayList<String>> d64_entries_map;
	ArrayList<String> currentList;
	File currFile;
	int d64_entries_start;
	
	public C64ImportView() {
		super(new String [] { "d64", "prg"/*, "t64"*/ });
		d64_entries_map = new HashMap<File, ArrayList<String>>();
	}

	@Override
	public ArrayList<String> getExtendedList(Application application, File file) {
		
		currFile = file;
		ArrayList<String> d64_entries = null;
		if (file.getName().toLowerCase().endsWith("d64")) {
			d64_entries_start = 1;
			d64_entries = (ArrayList<String> ) d64_entries_map.get(file);
			if (d64_entries == null) {
				d64_entries = new ArrayList<String>();
				d64_entries_map.put(file, d64_entries);
				try {
					RandomAccessFile raf = new RandomAccessFile(file, "r");
					int u = 91650;
					byte data [] = new byte [16];
					while (u < 93442) {
						raf.seek(u);
						byte b = raf.readByte();
						if (b != 0) {
							raf.seek(u+3);
							raf.read(data);
							if (data[0] > 0) {
								int max = data.length;
								while (max > 1 && data[max-1] < 0) {
									max--;
								}
								d64_entries.add(new String(data, 0, max).toLowerCase());
							}
						}
						u+= 32;
					}
					raf.close();
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		} else
			d64_entries_start = -1;
		
		currentList = new ArrayList<String>();
		currentList.add(application.getString(R.string.load_first_disk_entry));
		if (d64_entries_start > -1)
			d64_entries_start++;
		File snap = new File(file.getAbsolutePath() + ".sav");
		if (snap.exists()) {
			currentList.add(application.getString(R.string.load_snapshot_found));
			if (d64_entries_start > -1)
				d64_entries_start++;
		}
		if (d64_entries != null) {
			for(int i=0;i<d64_entries.size();i++) {
				currentList.add(application.getString(R.string.load_specific_program) + " #" + (i+1) + ": " + d64_entries.get(i).replace('/', ' '));
			}
		}
			
		return currentList;
	}


	public String getExtra2(int position) {
		if (currFile != null) {
			ArrayList<String> d64_entries = (ArrayList<String> ) d64_entries_map.get(currFile);
			if (d64_entries != null && position - d64_entries_start >= 0) {
				return d64_entries.get(position - d64_entries_start);
			}
		}
		return null;
	}
	
	public int getIcon(int position) {
		if (position == 1)
			return R.drawable.cdrwblank32;
		else if (position == 2 && (d64_entries_start == -1 || d64_entries_start>=3)) {
			return R.drawable.filesave32;
		} else if (position > 2 && d64_entries_start>=3) {
			return R.drawable.agtsoftware32;
		}
		return R.drawable.file;
	}
}
