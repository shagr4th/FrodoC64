
package org.ab.c64;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.nio.ShortBuffer;

import org.ab.controls.GameKeyListener;
import org.ab.controls.VirtualKeypad;
import org.ab.nativelayer.ImportView;
import org.ab.nativelayer.MainActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.inputmethodservice.Keyboard;
import android.os.Bundle;
import android.os.Debug;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;


public class FrodoC64 extends MainActivity implements GameKeyListener
{
	static final private int LOAD_ID = Menu.FIRST +1;
	static final private int RESET_ID = Menu.FIRST +2;
	static final private int QUIT_ID = Menu.FIRST +3;
	static final private int LOADS_ID = Menu.FIRST +4;
	static final private int SAVES_ID = Menu.FIRST +5;
	static final private int SETTINGS_ID = Menu.FIRST +6;
	static final private int CHANGE_J_PORT_ID = Menu.FIRST +7;
	static final private int KEYB_ID = Menu.FIRST +8;
	static final private int SHOWHIDEKEYB_ID = Menu.FIRST +9;
	static final private int HELP_ID = Menu.FIRST +10;
	
	//private static final int DIALOG_SINGLE_CHOICE = 5;
	
	private int fs = 2;
	private boolean num_locked = false;
	//private short silent [] = new short [512];
	private boolean debug;
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        readyToGo() ;
    }

    public native int startEmulator();
    public native int stopEmulator();
    public native void pause();
    public native void resume();
    public native void reset();
    public native void restore();
    public native void loadImage(String filename);
    public native void loadSnapshot(String filename);
    public native void saveSnapshot(String filename);
    public native void load81(int i);
    public native void loadX81(String programName, String driveName, int driveNamePresent);
    public native int setBorder(int border);
    public native int setSound(int sound);
    public native int setFS(int fs);
    public native int set1541(int enable1541);
    public native int setNumLock(int a);
    public native void registerClass(FrodoC64 instance);
    public native int fillScreenData(ShortBuffer buf, int width, int height, int keycode, int keycode2);
    public native int initScreenData(ShortBuffer buf, int width, int height, int keycode, int keycode2, int border);
    public native int sendKey(int keycode);
    
    private boolean frodopc;
    private boolean border;
    
    public boolean isBorder() {
		return border;
	}

	public void readyToGo() {
    	SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
    	frodopc = sp.getBoolean("frodopc", true);
    	System.loadLibrary(frodopc?"frodoc65":"frodoc64");
    	//System.loadLibrary("frodoc64");
    	initView();
    	switchKeyboard(1, true); // qwerty by default
    	Toast.makeText(this, R.string.keyb_mode_text, Toast.LENGTH_SHORT).show();
        majSettings(sp);
        if (debug)
        	Debug.startMethodTracing("calc");
        /*File sd = new File("/sdcard");
		if (sd.exists() && sd.isDirectory()) {
			File c64Dir = new File("/sdcard/.frodo_c64/");
			c64Dir.mkdir();
			String assets[];
			try {
				assets = getResources().getAssets().list( "" );
				for( int i = 0 ; i < assets.length ; i++ ) {
					if (assets[i].endsWith("d64") || assets[i].endsWith("prg")) {
						File fout = new File(c64Dir, assets[i]);
						if (!fout.exists()) {
							Log.i("frodoc64", "Writing " + assets[i]);
							FileOutputStream out = new FileOutputStream(fout);
							InputStream in = getResources().getAssets().open(assets[i]);
							byte buffer [] = new byte [8192];
							int n = -1;
							while ((n=in.read(buffer)) > -1) {
								out.write(buffer, 0, n);
							}
							out.close();
							in.close();
						}
					}
				}
			} catch (IOException e) {
				e.printStackTrace();
			}
			
		}*/
    }
    
    public void drawled(int num, int state) {
    	//tv.setText("" + state);
    	//tv.setVisibility(state>0?View.VISIBLE:View.INVISIBLE);
    }
    
    public int getEvent() {
    	if (mainView !=null)
    		return mainView.waitEvent();
    	return -1;
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);

        // We are going to create two menus. Note that we assign them
        // unique integer IDs, labels from our string resources, and
        // given them shortcuts.
        //menu.add(0, RESET_ID, 0, R.string.reset);
        menu.add(0, LOAD_ID, 0, R.string.load_image);
        menu.add(0, RESET_ID, 0, R.string.reset);
        menu.add(0, QUIT_ID, 0, R.string.quit);
        menu.add(0, LOADS_ID, 0, R.string.load_snap);
        menu.add(0, SAVES_ID, 0, R.string.save_snap);
        menu.add(0, SETTINGS_ID, 0, R.string.settings);
        menu.add(0, KEYB_ID, 0, R.string.joy_mode);
        menu.add(0, CHANGE_J_PORT_ID, 0, R.string.change_joy_port);
        menu.add(0, SHOWHIDEKEYB_ID, 0, R.string.showhidetouchcontrols);
         menu.add(0, HELP_ID, 0, R.string.help);
        
        return true;
    }
    
    @Override
    public boolean onMenuItemSelected(int featureId, MenuItem item) {
        switch (item.getItemId()) {
        	case LOAD_ID:
        		//showDialog(DIALOG_SINGLE_CHOICE);
        		Intent loadFileIntent = new Intent();
           		loadFileIntent.setClass(this, ImportView.class);
           		loadFileIntent.putExtra("import", new C64ImportView());
           		startActivityForResult(loadFileIntent, LOAD_ID);
           		return false;
        	case RESET_ID:
        		reset();
        		break;
        	case QUIT_ID:
        		if (debug)
        		Debug.stopMethodTracing();
  	           stopEmulator();
  	           if (mainView != null)
        		mainView.stop();
        		finish();
 	           return true;
        	case SETTINGS_ID:
           		Intent settingsIntent = new Intent();
           		settingsIntent.setClass(this, Settings.class);
           		startActivityForResult(settingsIntent, SETTINGS_ID);
           		
           		break;
        	case KEYB_ID:
        		if (mainView != null) {
        			
        			if (currentKeyboardLayout == 0) {
        				switchKeyboard(1, true);
        				Toast.makeText(this, R.string.keyb_mode_text, Toast.LENGTH_SHORT).show();
        			}
        			else if (currentKeyboardLayout > 0) {
        				switchKeyboard(0, false);
        				Toast.makeText(this, R.string.joy_mode_text, Toast.LENGTH_SHORT).show();
        			}
        			
        			if (currentKeyboardLayout == 0)
        				item.setTitle(R.string.keyb_mode);
        			else
        				item.setTitle(R.string.joy_mode);
        			
        			
        		}
        		break;
        	case CHANGE_J_PORT_ID:
        		num_locked = !num_locked;
        		setNumLock(num_locked?1:0);
        		
        		CharSequence text = "Joystick on port 2";
        		int duration = Toast.LENGTH_SHORT;

        		if (num_locked) {
        			text = "Joystick on port 1";
        		}
        		
        		Toast toast = Toast.makeText(this, text, duration);
        		toast.show();
        		
        		break;
        	case HELP_ID:
        		//showDialog(DIALOG_HELP);
        		Intent helpIntent = new Intent();
        		helpIntent.setClass(this, C64Help.class);
           		startActivityForResult(helpIntent, HELP_ID);
        		break;
        	case SHOWHIDEKEYB_ID:
        		manageTouchKeyboard(true);
        		break;
        	case LOADS_ID:
        		if (currentFile != null && currentFile.exists()) {
        			File snap = new File(currentFile + ".sav");
        			if (snap.exists()) {
        				loadImage(currentFile.getAbsolutePath());
        				loadC64Snap(snap);
        			} else {
            			toast = Toast.makeText(this, R.string.no_snap_found, Toast.LENGTH_SHORT);
            			toast.show();
            		}
        		} else {
        			toast = Toast.makeText(this, R.string.no_image_loaded, Toast.LENGTH_SHORT);
        			toast.show();
        		}
        		break;
        	case SAVES_ID:
        		if (currentFile != null && currentFile.exists()) {
        			File snap = new File(currentFile + ".sav");
        			saveSnapshot(snap.getAbsolutePath());
        			snap = new File(currentFile + ".and.sav");
    				
    					try {
    						FileOutputStream fout = new FileOutputStream(snap);
    						if (num_locked) {
    							fout.write("port1".getBytes());
    						}
	    					fout.write((";" + (frodopc?"mode=pc":"mode=normal")).getBytes());
	    					fout.close();
    					} catch (Exception e) {}
    				
        		} else {
        			toast = Toast.makeText(this, R.string.no_image_loaded, Toast.LENGTH_SHORT);
        			toast.show();
        		}
        		break;
        }
        return super.onMenuItemSelected(featureId, item);
    }
    
    private void loadC64Snap(File snap) {
    	File snapAnd = new File(currentFile + ".and.sav");
    	
    	if (snapAnd.exists()) {
	    	try {
				BufferedReader br = new BufferedReader(new FileReader(snapAnd));
				String line = null;
				while ((line=br.readLine()) != null) {
					if (line.contains("port1")) {
						num_locked = true;
						setNumLock(1);
					} else {
						num_locked = false;
						setNumLock(0);
					}
				}
				br.close();
			} catch (IOException e) {
			}
    	}
		
		loadSnapshot(snap.getAbsolutePath());
		
		
    }
    
    private File currentFile;
    
    @Override
    protected void onActivityResult(final int requestCode, final int resultCode, final Intent extras) {
    	 Log.i(getID(), "requestCode: " + requestCode + " / " + resultCode);
     	super.onActivityResult(requestCode, resultCode, extras);
     	
     	if (resultCode == RESULT_OK) {
     		switch (requestCode) {
 	    		case LOAD_ID: {
 	   				final String filename = extras.getStringExtra("currentFile");
 	   				if (filename != null) {
 	   					File f = new File(filename);
	   					if (f.exists()) {
	   						currentFile = f;
	   						/*File snap = new File(currentFile + ".sav");
	   						if (snap.exists()) {
	   	        				showDialog(DIALOG_SNAP);
	   	        			} else {
	   	        				loadC64Image(filename);
	   	        				onResume();
	   	        			}*/
	   						String extra1 = extras.getStringExtra("extra1");
	   						int position = extras.getIntExtra("extra2", -1);
	   						if (position == 1) {
	   							loadC64Image(filename, true, null);
	   						} else if (position > 1 && extra1 !=null && extra1.contains("state")) { // CRAPPYYYYY
	   							 File snap = new File(currentFile + ".sav");
	   			        		  if (snap.exists()) {
	   			        			loadImage(filename);
	   			        			   loadC64Snap(snap);
	   			        		  }
	   			        	   
	   						} else if (position > 1 && extra1 != null) {
	   							// disk browser
	   							loadC64Image(extra1, false, filename);
	   						}
   	        				onResume();
	   					}
 	   				}
 	   				
 	   				break;
    			}
 	    		case SETTINGS_ID: {
 	    			SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
 	    			majSettings(sp);
 	    		}
     		}
     	}
    }
    
    private void loadC64Image(String filename, boolean first_entry, String driveName) {
    	if ((filename.toLowerCase().endsWith(".d64")) && first_entry) {
			loadImage(filename);
			load81(1);
		} else if (filename.toLowerCase().endsWith(".t64")) {
			loadImage(filename);
			load81(1);
		} else {
			loadX81(filename, driveName, driveName!=null?1:0);
		}
    }
    
    //public boolean virtual;
    public static int default_keycodes [] = {  KeyEvent.KEYCODE_P, KeyEvent.KEYCODE_DPAD_CENTER, KeyEvent.KEYCODE_R, KeyEvent.KEYCODE_C, KeyEvent.KEYCODE_D, KeyEvent.KEYCODE_F, KeyEvent.KEYCODE_E, KeyEvent.KEYCODE_T, KeyEvent.KEYCODE_X, KeyEvent.KEYCODE_V,
    	KeyEvent.KEYCODE_0, KeyEvent.KEYCODE_1, KeyEvent.KEYCODE_2, KeyEvent.KEYCODE_3, KeyEvent.KEYCODE_4, KeyEvent.KEYCODE_5, KeyEvent.KEYCODE_6, KeyEvent.KEYCODE_7, KeyEvent.KEYCODE_8,
    	KeyEvent.KEYCODE_Y, KeyEvent.KEYCODE_B, KeyEvent.KEYCODE_I, KeyEvent.KEYCODE_O,
    	KeyEvent.KEYCODE_U, KeyEvent.KEYCODE_N, KeyEvent.KEYCODE_H, KeyEvent.KEYCODE_J};
    public static String default_keycodes_string [] = { "Fire", "Alt.Fire" , "Up", "Down", "Left", "Right", "UpLeft", "UpRight", "DownLeft", "DownRight", "Run/Stop", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "CTRL", "Restore", "C=", "ESC", "Cursor UP", "Cursor DOWN", "Cursor LEFT", "Cursor RIGHT"};
    public static int current_keycodes [];
    
    public void onSharedPreferenceChanged(SharedPreferences prefs, String key) {
    	if ("useInputMethod".equals(key))
    	getWindow().setFlags(prefs.getBoolean(key, false) ?
				0 : WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM,
				WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    }
	
    public void majSettings(SharedPreferences sp) {
    	border = sp.getBoolean("border", false);
    	sound = sp.getBoolean("sound", true);
    	setSound(sound?1:0);
        String f = sp.getString("frameskip", "3");
        fs = Integer.parseInt(f);
        setFS(fs);
        setNumLock(num_locked?1:0);
        set1541(sp.getBoolean("1541", false)?1:0);
       // virtual = sp.getBoolean("virtual", false);
        if (mainView != null) {
        	current_keycodes = new int [default_keycodes.length];
        	for(int i=0;i<default_keycodes.length;i++)
        		current_keycodes[i] = sp.getInt("key." + default_keycodes_string[i], default_keycodes[i]);
        }
        onSharedPreferenceChanged(sp, "useInputMethod");
    }
    
    

	@Override
	protected void onPause() {
		super.onPause();
		pause();
		pauseAudio();
	}

	public void emulatorReady() {
		Log.i(getID(), "emulatorReady");
		if (mainView  != null)
			mainView.emulatorReady();
    }

	@Override
	protected void onResume() {
		super.onResume();
		resume();
		playAudio();
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		stopEmulator();
		
	}
	
	private static final int DIALOG_HELP = 1;
	private static final int DIALOG_SNAP = 2;
	
	@Override
	protected Dialog onCreateDialog(int id) {
		switch (id) {
		   case DIALOG_HELP: return new AlertDialog.Builder(FrodoC64.this)
	       .setIcon(R.drawable.alert_dialog_icon)
	       .setTitle(R.string.help)
	       .setMessage(R.string.help_msg)
	       .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
	           public void onClick(DialogInterface dialog, int whichButton) {
	        	   onResume();
	           }
	       })
	       .create();
		   case DIALOG_SNAP: return new AlertDialog.Builder(FrodoC64.this)
	       .setIcon(R.drawable.alert_dialog_icon)
	       .setTitle(R.string.snapshot_found)
	       .setMessage(R.string.found_snapshot_summary)
	       .setPositiveButton(R.string.start_load_snap, new DialogInterface.OnClickListener() {
	           public void onClick(DialogInterface dialog, int whichButton) {
	        	   if (currentFile != null) {
	        		   File snap = new File(currentFile + ".sav");
	        		   if (snap.exists()) {
	        			   loadImage(currentFile.getAbsolutePath());
	        			   loadC64Snap(snap);
	        		   }
	        	   }
	        	   onResume();
	           }
	       })
	       .setNegativeButton(R.string.start_fresh, new DialogInterface.OnClickListener() {
	           public void onClick(DialogInterface dialog, int whichButton) {
	        	   if (currentFile != null)
	        		   loadC64Image(currentFile.getAbsolutePath(), true, null);
	        	   onResume();
	           }
	       })
	       .create();
		   /*case DIALOG_SINGLE_CHOICE:
	            return new AlertDialog.Builder(FrodoC64.this)
	                .setIcon(R.drawable.alert_dialog_icon)
	                .setTitle(R.string.choose_rom_choice)
	                .setSingleChoiceItems(R.array.load_rom_choice, 0, new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {

	                    	which = whichButton;
	                    	
	                    }
	                }).setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {

	                        if (which == 0) {
	                        	Intent loadFileIntent = new Intent();
	                       		loadFileIntent.setClass(getApplicationContext(), ImportView.class);
	                       		loadFileIntent.putExtra("import", new C64ImportView());
	                       		startActivityForResult(loadFileIntent, LOAD_ID);
	                        } else if (which == 1) {
	                        	Intent loadFileIntent = new Intent();
	                       		loadFileIntent.setClass(getApplicationContext(), ImportView.class);
	                       		loadFileIntent.putExtra("import", new C64ImportView());
	                       		loadFileIntent.putExtra("specificDir", "/sdcard/.frodo_c64/");
	                       		startActivityForResult(loadFileIntent, LOAD_ID);
	                        }
	                    }
	                }).setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {

	                        
	                    }
	                }).create();*/
		   }
		   return null;
	}
	
	
	private int matrix (int a, int b) {
		return (((a) << 3) | (b));
	}
	
	@Override
	public void startEmulation() {
		registerClass(this);
		startEmulator();
	}
	
	
	protected String getID() {
		return "frodoc64";
	}

	@Override
	protected int[] getKeyLayouts(int orientation) {
		if (orientation == Configuration.ORIENTATION_LANDSCAPE)
			return new int [] { R.xml.c64_joystick_landscape, R.xml.c64_qwerty, R.xml.c64_qwerty2 };
		return new int [] { R.xml.c64_joystick, R.xml.c64_qwerty, R.xml.c64_qwerty2 };
	}

	@Override
	protected int getHeight() {
		return border?272:200;
	}

	@Override
	protected int getWidth() {
		return border?384:320;
	}

	@Override
	protected boolean isOpenGL() {
		return false;
	}
	
	@Override
	protected void manageOnKey(int c) {
		if (c == Keyboard.KEYCODE_MODE_CHANGE) {
			// switch layout
			if (currentKeyboardLayout == 1)
				switchKeyboard(2, true);
			else if (currentKeyboardLayout == 2)
				switchKeyboard(1, true);
		} 
	}
	
	@Override
	protected void manageTouchKeyboard(boolean switchVisibility) {

		if (theKeyboard != null) {
		
			if (switchVisibility) {
				theKeyboard.setVisibility(theKeyboard.getVisibility()!=View.VISIBLE?View.VISIBLE:View.INVISIBLE);
				theKeyboard.requestLayout();
			} else if (orientation == Configuration.ORIENTATION_LANDSCAPE && hardKeyboardHidden == Configuration.HARDKEYBOARDHIDDEN_NO) {
				theKeyboard.setVisibility(View.INVISIBLE);
			} else {
				theKeyboard.setVisibility(View.VISIBLE);
			}
			
			SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
			boolean oldJoystick = sp.getBoolean("oldJoystick", false);
			if (oldJoystick) {
				boolean shift = theKeyboard.getVisibility()==View.VISIBLE && orientation == Configuration.ORIENTATION_LANDSCAPE && currentKeyboardLayout == 0;  //&& hardKeyboardHidden != Configuration.HARDKEYBOARDHIDDEN_NO
				if (mainView != null)
					mainView.shiftImage(shift?160:0);
				vKeyPad = null;
			} else {
				if (mainView != null)
					mainView.shiftImage(0);
				boolean shift = theKeyboard.getVisibility()==View.VISIBLE && currentKeyboardLayout == 0;
				if (mainView != null && shift) { // on vire le keypad Android pour mettre celui de yongzh
					theKeyboard.setVisibility(View.INVISIBLE);
					vKeyPad = new VirtualKeypad(mainView, this, R.drawable.dpad5, R.drawable.button);
					if (mainView.getWidth() > 0)
						vKeyPad.resize(mainView.getWidth(), mainView.getHeight());
				}
				else
					vKeyPad = null;
			}
		}
	}
	
	private int currentKeyStates = 0;
	
	@Override
	public void onGameKeyChanged(int keyStates) {
		if (mainView != null) {
			manageKey(keyStates, VirtualKeypad.BUTTON, current_keycodes[0]);
			manageKey(keyStates, VirtualKeypad.UP, current_keycodes[2]);
			manageKey(keyStates, VirtualKeypad.DOWN, current_keycodes[3]);
			manageKey(keyStates, VirtualKeypad.LEFT, current_keycodes[4]);
			manageKey(keyStates, VirtualKeypad.RIGHT, current_keycodes[5]);
			manageKey(keyStates, VirtualKeypad.UP | VirtualKeypad.LEFT, current_keycodes[6]);
			manageKey(keyStates, VirtualKeypad.UP | VirtualKeypad.RIGHT, current_keycodes[7]);
			manageKey(keyStates, VirtualKeypad.DOWN | VirtualKeypad.LEFT, current_keycodes[8]);
			manageKey(keyStates, VirtualKeypad.DOWN | VirtualKeypad.RIGHT, current_keycodes[9]);
		}
		
		currentKeyStates = keyStates;
		
	}

	private void manageKey(int keyStates, int key, int press) {
		 if ((keyStates & key) == key && (currentKeyStates & key) == 0) {
			// Log.i("FC64", "down: " + press );
			 mainView.actionKey(true, press);
		 } else if ((keyStates & key) == 0 && (currentKeyStates & key) == key) {
			// Log.i("FC64", "up: " + press );
			 mainView.actionKey(false, press);
		 }
	}
	
	private boolean backKeyTracking;
	
	@Override
	protected int getRealKeycode(int c, boolean down) {
		
		if (c == KeyEvent.KEYCODE_MENU) {
			backKeyTracking = false;
			return Integer.MIN_VALUE;
		}
		
		if (c == KeyEvent.KEYCODE_BACK && down) {
			backKeyTracking = true;
		} else if (c == KeyEvent.KEYCODE_BACK && !down && backKeyTracking) {
			// switch layout
			backKeyTracking = false;
			if (currentKeyboardLayout == 0) {
				switchKeyboard(1, true);
				Toast.makeText(this, R.string.keyb_mode_text, Toast.LENGTH_SHORT).show();
			}
			else if (currentKeyboardLayout > 0) {
				switchKeyboard(0, false);
				Toast.makeText(this, R.string.joy_mode_text, Toast.LENGTH_SHORT).show();
			}
			manageTouchKeyboard(false);
			return Integer.MAX_VALUE;
		} else {
			backKeyTracking = false;
			boolean phj = currentKeyboardLayout == 0;
			boolean phk = currentKeyboardLayout > 0;
			if ((phj && c == current_keycodes[0]) || (phj && c == current_keycodes[1]) || (phj && c == 1005)) {
				return 516;
			} else if ((phj && c == current_keycodes[6]) || c == 1001) { // joystick
				return 521;
			} else if ((phj && c == current_keycodes[2]) || c == 1002 || (phj && c == KeyEvent.KEYCODE_DPAD_UP)) {
				return 517;
			} else if ((phj && c == current_keycodes[7]) || c == 1003) {
				return 522;
			} else if ((phj && c == current_keycodes[4]) || c == 1004 || (phj && c == KeyEvent.KEYCODE_DPAD_LEFT)) {
				return 519;
			} else if ((phj && c == current_keycodes[5]) || c == 1006 || (phj && c == KeyEvent.KEYCODE_DPAD_RIGHT)) {
				return 520;
			} else if ((phj && c == current_keycodes[8]) || c == 1007) {
				return 523;
			} else if ((phj && c == current_keycodes[3]) || c == 1008 || (phj && c == KeyEvent.KEYCODE_DPAD_DOWN)) {
				return 518;
			} else if ((phj && c == current_keycodes[9]) || c == 1009) {
				return 524;
			} else if ((phj && c == current_keycodes[10]) || c == 11010) { // R/S
				return matrix(7,7);
			} else if ((phj && c == current_keycodes[11]) || c == 11011) { // F1
				return matrix(0,4);
			} else if ((phj && c == current_keycodes[12]) || c == 11012) { 
				return matrix(0,4) | 0x80;
			} else if ((phj && c == current_keycodes[13]) || c == 11013) { 
				return matrix(0,5);
			} else if ((phj && c == current_keycodes[14]) || c == 11014) { 
				return matrix(0,5) | 0x80;
			} else if ((phj && c == current_keycodes[15]) || c == 11015) { 
				return matrix(0,6);
			} else if ((phj && c == current_keycodes[16]) || c == 11016) { 
				return matrix(0,6) | 0x80;
			} else if ((phj && c == current_keycodes[17]) || c == 11017) { 
				return matrix(0,3);
			} else if ((phj && c == current_keycodes[18]) || c == 11018) { 
				return matrix(0,3) | 0x80;
			} else if ((phj && c == current_keycodes[19]) || c == 20221) {
				return  matrix(7,2);
			} else if ((phj && c == current_keycodes[20]) || c == 20222) {
				restore();
				return  matrix(7,5);
			} else if ((phj && c == current_keycodes[21]) || c == 20223) {
				return  matrix(7,5);
			} else if ((phj && c == current_keycodes[22]) || c == 20224) {
				return  matrix(7,1);
			} else if ((phj && c == current_keycodes[23]) || c == 20225) {
				return (matrix(1,7) << 16) + matrix(0,7);
			} else if ((phj && c == current_keycodes[24]) || c == 20226) {
				return matrix(0,7);
			} else if ((phj && c == current_keycodes[25]) || c == 20227) {
				return (matrix(1,7) << 16) + matrix(0,2);
			} else if ((phj && c == current_keycodes[26]) || c == 20228) {
				return matrix(0,2);
			} 
			
			if ((phk && c == KeyEvent.KEYCODE_1) || c == 10011) { // 1
				return matrix(7, 0);
			} else if ((phk && c == KeyEvent.KEYCODE_2) || c == 10012) { 
				return matrix(7,3);
			} else if ((phk && c == KeyEvent.KEYCODE_3) || c == 10013) { 
				return matrix(1,0);
			} else if ((phk && c == KeyEvent.KEYCODE_4) || c == 10014) { 
				return matrix(1,3);
			} else if ((phk && c == KeyEvent.KEYCODE_5) || c == 10015) { 
				return  matrix(2,0);
			} else if ((phk && c == KeyEvent.KEYCODE_6) || c == 10016) { 
				return matrix(2,3);
			} else if ((phk && c == KeyEvent.KEYCODE_7) || c == 10017) { 
				return  matrix(3,0);
			} else if ((phk && c == KeyEvent.KEYCODE_8) || c == 10018) { 
				return matrix(3,3);
			} else if ((phk && c == KeyEvent.KEYCODE_9) || c == 10019) {
				return  matrix(4,0);
			} else if ((phk && c == KeyEvent.KEYCODE_0) || c == 10020) {
				return matrix(4,3);
			} else if ((c == KeyEvent.KEYCODE_SPACE) || c == 11019) {
				return matrix(7,4);
			} else if ((phk && c == KeyEvent.KEYCODE_A) || c == 10031) {
				return matrix(1,2);
			} else if ((phk && c == KeyEvent.KEYCODE_B) || c == 10046) {
				return matrix(3,4);
			} else if ((phk && c == KeyEvent.KEYCODE_C) || c == 10044) {
				return matrix(2,4);
			} else if ((phk && c == KeyEvent.KEYCODE_D) || c == 10033) {
				return matrix(2,2);
			} else if ((phk && c == KeyEvent.KEYCODE_E) || c == 10023) {
				return matrix(1,6);
			} else if ((phk && c == KeyEvent.KEYCODE_F) || c == 10034) {
				return matrix(2,5);
			} else if ((phk && c == KeyEvent.KEYCODE_G) || c == 10035) {
				return matrix(3,2);
			} else if ((phk && c == KeyEvent.KEYCODE_H) || c == 10036) {
				return matrix(3,5);
			} else if ((phk && c == KeyEvent.KEYCODE_I) || c == 10028) {
				return matrix(4,1);
			} else if ((phk && c == KeyEvent.KEYCODE_J) || c == 10037) {
				return matrix(4,2);
			} else if ((phk && c == KeyEvent.KEYCODE_K) || c == 10038) {
				return matrix(4,5);
			} else if ((phk && c == KeyEvent.KEYCODE_L) || c == 10039) {
				return matrix(5,2);
			} else if ((phk && c == KeyEvent.KEYCODE_M) || c == 10048) {
				return matrix(4,4);
			} else if ((phk && c == KeyEvent.KEYCODE_N) || c == 10047) {
				return matrix(4,7);
			} else if ((phk && c == KeyEvent.KEYCODE_O) || c == 10029) {
				return matrix(4,6);
			} else if ((phk && c == KeyEvent.KEYCODE_P) || c == 10030) {
				return matrix(5,1);
			} else if ((phk && c == KeyEvent.KEYCODE_Q) || c == 10021) {
				return matrix(7,6);
			} else if ((phk && c == KeyEvent.KEYCODE_R) || c == 10024) {
				return matrix(2,1);
			} else if ((phk && c == KeyEvent.KEYCODE_S) || c == 10032) {
				return matrix(1,5);
			} else if ((phk && c == KeyEvent.KEYCODE_T) || c == 10025) {
				return matrix(2,6);
			} else if ((phk && c == KeyEvent.KEYCODE_U) || c == 10027) {
				return matrix(3,6);
			} else if ((phk && c == KeyEvent.KEYCODE_V) || c == 10045) {
				return matrix(3,7);
			} else if ((phk && c == KeyEvent.KEYCODE_W) || c == 10022) {
				return matrix(1,1);
			} else if ((phk && c == KeyEvent.KEYCODE_X) || c == 10043) {
				return matrix(2,7);
			} else if ((phk && c == KeyEvent.KEYCODE_Y) || c == 10026) {
				return matrix(3,1);
			} else if ((phk && c == KeyEvent.KEYCODE_Z) || c == 10042) {
				return matrix(1,4);
			} else if ((c == KeyEvent.KEYCODE_ENTER) || c == 10049) {
				return matrix(0,1);
			} else if ((phk && c == KeyEvent.KEYCODE_COMMA) || c == 20133) {
				return matrix(5,7);
			} else if ((phk && c == KeyEvent.KEYCODE_PERIOD) || c == 20134) {
				return matrix(5,4);
			} else if ((phk && c == KeyEvent.KEYCODE_PLUS) || c == 20122) {
				return matrix(5,0);
			} else if ((phk && c == KeyEvent.KEYCODE_MINUS) || c == 20123) {
				return matrix(5,3);
			} else if ((phk && c == KeyEvent.KEYCODE_AT) || c == 20125) {
				return matrix(5,6);
			} else if ((phk && c == KeyEvent.KEYCODE_EQUALS) || c == 20128) {
				return matrix(6,5);
			} else if ((phk && c == KeyEvent.KEYCODE_DEL) || c == 10040) {
				return matrix(0,0);
			} else if (phk && c == KeyEvent.KEYCODE_DPAD_DOWN) {
				return matrix(0,7);
			} else if (phk && c == KeyEvent.KEYCODE_DPAD_UP) {
				return matrix(0,7) | 0x80;
			} else if (phk && c == KeyEvent.KEYCODE_DPAD_RIGHT) {
				return matrix(0,2);
			} else if (phk && c == KeyEvent.KEYCODE_DPAD_LEFT) {
				return matrix(0,2) | 0x80;
			} else if (c == 20111) {
				return (matrix(1,7) << 16) + matrix(7,0);
			} else if (c == 20112) {
				return (matrix(1,7) << 16) + matrix(7,3);
			} else if (c == 20113) {
				return  (matrix(1,7) << 16) + matrix(1,0);
			} else if (c == 20114) {
				return (matrix(1,7) << 16) + matrix(1,3);
			} else if (c == 20115) {
				return (matrix(1,7) << 16) + matrix(2,0);
			} else if (c == 20116) {
				return (matrix(1,7) << 16) + matrix(2,3);
			} else if (c == 20117) {
				return  (matrix(1,7) << 16) + matrix(3,0);
			} else if (c == 20118) {
				return  (matrix(1,7) << 16) + matrix(3,3);
			} else if (c == 20119) {
				return (matrix(1,7) << 16) + matrix(4,0);
			} else if (c == 20120) {
				return (matrix(1,7) << 16) + matrix(6,7);
			} else if (c == 20121) {
				return matrix(7,1);
			} else if (c == 20124) {
				return matrix(6,6);
			} else if (c == 20126) {
				return  matrix(6,1);
			} else if (c == 20129) {
				return matrix(6,7);
			} else if (c == 20131) {
				return  (matrix(1,7) << 16) + matrix(5,7);
			} else if (c == 20132) {
				return  (matrix(1,7) << 16) + matrix(5,4);
			} else if (c == 20135) {
				return matrix(6,2);
			} else if (c == 20136) {
				return  matrix(5,5);
			} else if ((phk && c == KeyEvent.KEYCODE_AT) || c == 10031) {
				return matrix(5,6);
			} /*else if (phk && c == KeyEvent.KEYCODE_SHIFT_LEFT) {
				return matrix(1,7);
			} else if (phk && c == KeyEvent.KEYCODE_SHIFT_RIGHT) {
				return matrix(6,4);
			} */
		}
		
		
		/*
		  C64 keyboard matrix:

		    Bit 7   6   5   4   3   2   1   0
		  0    CUD  F5  F3  F1  F7 CLR RET DEL
		  1    SHL  E   S   Z   4   A   W   3
		  2     X   T   F   C   6   D   R   5
		  3     V   U   H   B   8   G   Y   7
		  4     N   O   K   M   0   J   I   9
		  5     ,   @   :   .   -   L   P   +
		  6     /   ^   =  SHR HOM  ;   *   �
		  7    R/S  Q   C= SPC  2  CTL  <-  1
		*/
		
		return Integer.MIN_VALUE;
	}

	
}
