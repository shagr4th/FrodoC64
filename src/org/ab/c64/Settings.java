package org.ab.c64;

import org.ab.nativelayer.KeyPreference;

import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceScreen;
import android.view.KeyEvent;

public class Settings extends PreferenceActivity {
	
	 @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        setPreferenceScreen(createPreferenceHierarchy());
        
       
    }
	 
	 private PreferenceScreen createPreferenceHierarchy() {
	        // Root
	        PreferenceScreen root = getPreferenceManager().createPreferenceScreen(this);
	        root.setTitle(R.string.settings);
	        PreferenceCategory inlinePrefCat = new PreferenceCategory(this);
	        inlinePrefCat.setTitle(R.string.general_settings);
	        root.addPreference(inlinePrefCat);
	        
	        CheckBoxPreference togglePref = new CheckBoxPreference(this);
	        togglePref.setKey("sound");
	        togglePref.setTitle(R.string.sound);
	        togglePref.setDefaultValue(true);
	        togglePref.setSummary(R.string.sound_summary);
	        inlinePrefCat.addPreference(togglePref);
	        
	        CheckBoxPreference toggleBordersPref = new CheckBoxPreference(this);
	        toggleBordersPref.setKey("border");
	        toggleBordersPref.setTitle(R.string.show_borders);
	        toggleBordersPref.setDefaultValue(false);
	        toggleBordersPref.setSummary(R.string.show_borders_summary);
	        inlinePrefCat.addPreference(toggleBordersPref);
	        
	        CheckBoxPreference togglePC = new CheckBoxPreference(this);
	        togglePC.setKey("frodopc");
	        togglePC.setTitle(R.string.frodopc);
	        togglePC.setDefaultValue(true);
	        togglePC.setSummary(R.string.frodopc_summary);
	        inlinePrefCat.addPreference(togglePC);
	        
	        CheckBoxPreference toggle1541Pref = new CheckBoxPreference(this);
	        toggle1541Pref.setKey("1541");
	        toggle1541Pref.setTitle(R.string.enable_1541);
	        toggle1541Pref.setDefaultValue(false);
	        toggle1541Pref.setSummary(R.string.enable_1541_summary);
	        inlinePrefCat.addPreference(toggle1541Pref);
	        
	        ListPreference fs1Pref = new ListPreference(this);
	        fs1Pref.setEntries(R.array.fs_entries);
	        fs1Pref.setEntryValues(R.array.fs_entries);
	        fs1Pref.setDefaultValue("3");
	        fs1Pref.setDialogTitle(R.string.frame_skip);
	        fs1Pref.setKey("frameskip");
	        fs1Pref.setTitle(R.string.frame_skip);
	        fs1Pref.setSummary(R.string.frame_skip_summary);
	        inlinePrefCat.addPreference(fs1Pref);
	        
	        
	        
	        PreferenceCategory portPrefCat = new PreferenceCategory(this);
	        portPrefCat.setTitle(R.string.mapping_settings);
	        root.addPreference(portPrefCat);
	        
	        CheckBoxPreference toggleInputMethodPref = new CheckBoxPreference(this);
	        toggleInputMethodPref.setKey("useInputMethod");
	        toggleInputMethodPref.setTitle(R.string.useInputMethodTitle);
	        toggleInputMethodPref.setDefaultValue(false);
	        toggleInputMethodPref.setSummary(R.string.useInputMethod);
	        portPrefCat.addPreference(toggleInputMethodPref);
	        
	        /*
	        CheckBoxPreference toggleFsjoystickPref = new CheckBoxPreference(this);
	        toggleFsjoystickPref.setKey("fsjoystick");
	        toggleFsjoystickPref.setTitle(R.string.enable_fsjoystick);
	        toggleFsjoystickPref.setDefaultValue(false);
	        toggleFsjoystickPref.setSummary(R.string.enable_fsjoystick_summary);
	        portPrefCat.addPreference(toggleFsjoystickPref);
	        */
	        PreferenceScreen screenPref = getPreferenceManager().createPreferenceScreen(this);
	        screenPref.setKey("screen_preference");
	        screenPref.setTitle(R.string.custom_mappings);
	        screenPref.setSummary(R.string.custom_mappings_summary);
	        portPrefCat.addPreference(screenPref);
	        
	       
	        ListPreference joyLayoutPref = new ListPreference(this);
	        joyLayoutPref.setEntries(R.array.joystick_layout_test);
	        joyLayoutPref.setEntryValues(R.array.joystick_layout);
	        joyLayoutPref.setDefaultValue("bottom_bottom");
	        joyLayoutPref.setDialogTitle(R.string.joystick_layout_test);
	        joyLayoutPref.setKey("vkeypadLayout");
	        joyLayoutPref.setTitle(R.string.joystick_layout_test);
	        portPrefCat.addPreference(joyLayoutPref);
	        
	        ListPreference joySizePref = new ListPreference(this);
	        joySizePref.setEntries(R.array.joystick_size);
	        joySizePref.setEntryValues(R.array.joystick_size);
	        joySizePref.setDefaultValue("medium");
	        joySizePref.setDialogTitle(R.string.joystick_layout_size);
	        joySizePref.setKey("vkeypadSize");
	        joySizePref.setTitle(R.string.joystick_layout_size);
	        portPrefCat.addPreference(joySizePref);
	        
	        ListPreference screenSizePref = new ListPreference(this);
	        screenSizePref.setEntries(R.array.screen_size);
	        screenSizePref.setEntryValues(R.array.screen_size);
	        screenSizePref.setDefaultValue("stretched");
	        screenSizePref.setDialogTitle(R.string.screen_size_text);
	        screenSizePref.setKey("scale");
	        screenSizePref.setTitle(R.string.screen_size_text);
	        portPrefCat.addPreference(screenSizePref);
	        
	        CheckBoxPreference toggleOldjoystickPref = new CheckBoxPreference(this);
	        toggleOldjoystickPref.setKey("oldJoystick");
	        toggleOldjoystickPref.setTitle(R.string.old_joystick);
	        toggleOldjoystickPref.setDefaultValue(false);
	        toggleOldjoystickPref.setSummary(R.string.old_joystick_summay);
	        portPrefCat.addPreference(toggleOldjoystickPref);
	       
	        for(int i=0;i<FrodoC64.default_keycodes.length;i++) {
	        	screenPref.addPreference(new KeyPreference(this, null, FrodoC64.default_keycodes_string[i], FrodoC64.default_keycodes[i], new int [] { KeyEvent.KEYCODE_BACK, KeyEvent.KEYCODE_MENU }));
	        }
	                
	        setResult(RESULT_OK);
	        
	        return root;
	 }

}
