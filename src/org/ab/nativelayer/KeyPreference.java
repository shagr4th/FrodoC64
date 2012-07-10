package org.ab.nativelayer;

import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.DialogInterface.OnKeyListener;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.preference.DialogPreference;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.TextView;

public class KeyPreference extends DialogPreference implements OnKeyListener {
	
	private Context context;
	private int deniedKeyCode [];
	//private int defaultKeyCode;
	private String prefKey;
	//private String prefLabel;

	public KeyPreference(Context context, AttributeSet attrs, String prefKey, int defaultKeyCode, int deniedKeyCode []) {
		super(context, attrs);
		this.context = context;
		this.deniedKeyCode = deniedKeyCode;
		setTitle(prefKey);
        this.prefKey = "key." + prefKey;
		//this.prefLabel = prefLabel;
		//this.defaultKeyCode = defaultKeyCode;
		setKey(this.prefKey);
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(context);
        int currentKeyCode = sp.getInt(this.prefKey, defaultKeyCode) ;
        setSummary(getLabel(currentKeyCode));
		setPositiveButtonText(null);
		setNegativeButtonText("Cancel");
	}

	@Override
	protected void onPrepareDialogBuilder(Builder builder) {
		LinearLayout layout = new LinearLayout(context); 
        layout.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT, 
        	LinearLayout.LayoutParams.WRAP_CONTENT));
        layout.setMinimumWidth(200);
        TextView tv = new TextView(context);
        tv.setText("Press a key!");
        layout.addView(tv);
        builder.setView(layout); 
        builder.setOnKeyListener(this);
        super.onPrepareDialogBuilder(builder);
	}

	public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
		if (event.getAction() == KeyEvent.ACTION_UP) {
			for(int d=0;d<deniedKeyCode.length;d++) {
				if (keyCode == deniedKeyCode[d])
					return false;
			}
			// maj pref
			SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(context);
			//sp.getInt(prefKey, defaultKeyCode);
			Editor e = sp.edit();
			e.putInt(prefKey, keyCode);
			e.commit();
			dialog.dismiss();
			setSummary(getLabel(keyCode));
			return true;
		}
		return false;
	}

	private String getLabel(int keycode) {
		switch (keycode) {
			case KeyEvent.KEYCODE_A: return "A";
			case KeyEvent.KEYCODE_B: return "B";
			case KeyEvent.KEYCODE_C: return "C";
			case KeyEvent.KEYCODE_D: return "D";
			case KeyEvent.KEYCODE_E: return "E";
			case KeyEvent.KEYCODE_F: return "F";
			case KeyEvent.KEYCODE_G: return "G";
			case KeyEvent.KEYCODE_H: return "H";
			case KeyEvent.KEYCODE_I: return "I";
			case KeyEvent.KEYCODE_J: return "J";
			case KeyEvent.KEYCODE_K: return "K";
			case KeyEvent.KEYCODE_L: return "L";
			case KeyEvent.KEYCODE_M: return "M";
			case KeyEvent.KEYCODE_N: return "N";
			case KeyEvent.KEYCODE_O: return "O";
			case KeyEvent.KEYCODE_P: return "P";
			case KeyEvent.KEYCODE_Q: return "Q";
			case KeyEvent.KEYCODE_R: return "R";
			case KeyEvent.KEYCODE_S: return "S";
			case KeyEvent.KEYCODE_T: return "T";
			case KeyEvent.KEYCODE_U: return "U";
			case KeyEvent.KEYCODE_V: return "V";
			case KeyEvent.KEYCODE_W: return "W";
			case KeyEvent.KEYCODE_X: return "X";
			case KeyEvent.KEYCODE_Y: return "Y";
			case KeyEvent.KEYCODE_Z: return "Z";
			case KeyEvent.KEYCODE_0: return "0";
			case KeyEvent.KEYCODE_1: return "1";
			case KeyEvent.KEYCODE_2: return "2";
			case KeyEvent.KEYCODE_3: return "3";
			case KeyEvent.KEYCODE_4: return "4";
			case KeyEvent.KEYCODE_5: return "5";
			case KeyEvent.KEYCODE_6: return "6";
			case KeyEvent.KEYCODE_7: return "7";
			case KeyEvent.KEYCODE_8: return "8";
			case KeyEvent.KEYCODE_9: return "9";
			case KeyEvent.KEYCODE_PERIOD: return ".";
			case KeyEvent.KEYCODE_COMMA: return ",";
			case KeyEvent.KEYCODE_SHIFT_LEFT: return "SHIFTL";
			case KeyEvent.KEYCODE_SHIFT_RIGHT: return "SHIFTR";
			case KeyEvent.KEYCODE_ALT_LEFT: return "ALTL";
			case KeyEvent.KEYCODE_ALT_RIGHT: return "ALTR";
			case KeyEvent.KEYCODE_SPACE: return "SPACE";
			case KeyEvent.KEYCODE_DEL: return "DEL";
			case KeyEvent.KEYCODE_DPAD_CENTER: return "D-PAD CENTER";
			case KeyEvent.KEYCODE_DPAD_LEFT: return "D-PAD LEFT";
			case KeyEvent.KEYCODE_DPAD_RIGHT: return "D-PAD RIGHT";
			case KeyEvent.KEYCODE_DPAD_UP: return "D-PAD UP";
			case KeyEvent.KEYCODE_DPAD_DOWN: return "D-PAD DOWN";
			case KeyEvent.KEYCODE_VOLUME_DOWN: return "VOL-";
			case KeyEvent.KEYCODE_VOLUME_UP: return "VOL+";
			case KeyEvent.KEYCODE_CAMERA: return "CAMERA";
			case KeyEvent.KEYCODE_SYM: return "SYM";
			case KeyEvent.KEYCODE_SEARCH: return "SEARCH";
			case KeyEvent.KEYCODE_AT: return "@";
			case KeyEvent.KEYCODE_SEMICOLON: return ";";
			case KeyEvent.KEYCODE_ENTER: return "ENTER";
			case KeyEvent.KEYCODE_BACK: return "BACK";
			case KeyEvent.KEYCODE_CALL: return "CALL";
			case KeyEvent.KEYCODE_ENDCALL: return "ENDCALL";
			case KeyEvent.KEYCODE_HOME: return "HOME";
			case KeyEvent.KEYCODE_TAB: return "TAB";
		}
		return "Code: " + keycode;
	}
	
	@Override
	protected void showDialog(Bundle state) {
		super.showDialog(state);

		final Dialog dialog = getDialog();
		if (dialog != null) {
			dialog.getWindow().clearFlags(
					WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
		}
	}

}
