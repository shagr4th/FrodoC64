package org.ab.nativelayer;


import org.ab.c64.R;
import org.ab.controls.VirtualKeypad;

import android.app.Activity;
import android.content.res.Configuration;
import android.inputmethodservice.Keyboard;
import android.inputmethodservice.KeyboardView;
import android.inputmethodservice.KeyboardView.OnKeyboardActionListener;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

public abstract class MainActivity extends Activity {

	protected boolean sound = true;
	public abstract void startEmulation();
	protected MainView mainView;
	protected static Thread emulationThread;
	protected VirtualKeypad vKeyPad = null;
	protected abstract String getID();
	protected abstract int getWidth();
	protected abstract int getHeight();
	protected abstract boolean isOpenGL();
	protected abstract int getRealKeycode(int c, boolean down);
	protected abstract int [] getKeyLayouts(int orientation);
	
	public static final int CANVAS_2D = 1;
	public static final int OPENGL = 2;
	
	protected void initView() {
		initView(isOpenGL()?OPENGL:CANVAS_2D);
	}
	
	
	protected void manageOnKey(int code) {
		
	}
	
	private int currentprimaryCode;
	
	public class theKeyboardActionListener implements OnKeyboardActionListener{

        public void onKey(int primaryCode, int[] keyCodes) {
        	manageOnKey(primaryCode);
        }

        public void onPress(int primaryCode) {
        	if (mainView != null) {
        		if (currentprimaryCode > -1)
        			mainView.actionKey(false, currentprimaryCode);
        		mainView.actionKey(true, primaryCode);
        		currentprimaryCode = primaryCode;
        	}
        }

        public void onRelease(int primaryCode) {
        	if (mainView != null) {
        		if (currentprimaryCode > -1 && currentprimaryCode == primaryCode)
        			mainView.actionKey(false, primaryCode);
        		else {
        			if (currentprimaryCode > -1)
        				mainView.actionKey(false, currentprimaryCode);
        			else
        				mainView.actionKey(false, primaryCode);
        			currentprimaryCode = -1;
        		}
        	}
        		
        }

        public void onText(CharSequence text) {}

        public void swipeDown() {}

        public void swipeLeft() {}

        public void swipeRight() {}

        public void swipeUp() {}};
	
	protected void initView(int draw_type) {
		requestWindowFeature(Window.FEATURE_NO_TITLE);
        requestWindowFeature(Window.FEATURE_PROGRESS);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN); 
        
        if (mainView == null) {
        	mainView = new MainView(this, null);
        	//if (draw_type == OPENGL)
        	//	mainView.setOpenGL(true);
            mainView.init(this);
        }
        
        
//        mainView = new MainView(this, null);
//        mainView.setOpenGL(draw_type != CANVAS_2D);
//        mainView.init(this);
//        if (draw_type == CANVAS_2D)
//        	setContentView(mainView);
//        else {
//	        mainGLView = new MainGLSurfaceView(this, mainView);
//	        mainGLView.requestFocus();
//	        mainGLView.setFocusableInTouchMode(true);
//	        setContentView(mainGLView);
//        }
        
       /* setContentView(R.layout.main);
		
        mainGLView = ((MainGLSurfaceView) findViewById(R.id.mainview));
        mainGLView.setR(mainView);
        mainGLView.requestFocus();
        mainGLView.setFocusableInTouchMode(true);
         
        theKeyboard = (KeyboardView) findViewById(R.id.EditKeyboard01);
		int key_layouts [] = getKeyLayouts();
		layouts = new Keyboard [key_layouts.length];
		for(int i=0;i<layouts.length;i++)
			layouts[i] = new Keyboard(this, key_layouts[i]);
        theKeyboard.setKeyboard(layouts[currentKeyboardLayout]);
        theKeyboard.setOnKeyboardActionListener(new theKeyboardActionListener());
        theKeyboard.setVisibility(View.VISIBLE);
        theKeyboard.setPreviewEnabled(false);
        */
        onConfigurationChanged(getResources().getConfiguration());
        
        
        
	}
	
	public int waitEvent() {
    	//return mainView.waitEvent();
		return 0;
    }
    
	@Override
	public boolean onMenuOpened(int featureId, Menu menu) {
		onPause();
		return super.onMenuOpened(featureId, menu);
	}

	@Override
	public void onOptionsMenuClosed(Menu menu) {
		onResume();
		super.onOptionsMenuClosed(menu);
	}
	
	 
	
	private AudioTrack audio;
    private boolean play;
    //private SoundThread soundThread;
    //private byte sound_copy [];
    
    public void initAudio(int freq, int bits, int sound_packet_length) {
    	if (audio == null && sound) {
    		audio = new AudioTrack(AudioManager.STREAM_MUSIC, freq, AudioFormat.CHANNEL_CONFIGURATION_MONO, bits == 8?AudioFormat.ENCODING_PCM_8BIT:AudioFormat.ENCODING_PCM_16BIT, freq==44100?32*1024:16*1024, AudioTrack.MODE_STREAM);
    		//soundThread = new SoundThread(freq);
    		//sound_copy = new byte [sound_packet_length*((bits==8)?1:2)];
    		audio.play();
    	}
    }
    
    public void pauseAudio() {
    	if (audio != null && sound && play) {
    		audio.pause();
    		Log.i(getID(), "audio paused");
    	}
    	
    }
    
    public void playAudio() {
    	if (audio != null && sound && play) {
    		audio.play();
    	}
    	
    }
    
    /*
    public void sendAudio(ByteBuffer bb) {
    	if (audio != null && sound) {
    		bb.get(sound_copy);
    		bb.position(0);
    		audio.write(sound_copy, 0, sound_copy.length);
    		if (!play) {
    			audio.play();
    			play = true;
    		}
    	}
    }
    */
    
    long t = System.currentTimeMillis();
	int frames;
	public void checkFPS() {

        frames++;
        
        if (frames % 20 == 0) {
        	long t2 = System.currentTimeMillis();
        	Log.i("frodoc64", "FPS: " + 20000 / (t2 - t));
        	t = t2;
        }
        
	}
    
    public void sendAudio(short data []) {
    	if (audio != null && sound) {
    		//Log.i("frodoc64", ">>>");
    		audio.write(data, 0, data.length);
    		
    		//checkFPS();
    		//Log.i("frodoc64", "<<<");
    		if (!play) {
    			
    			play = true;
    		}
    	}
    	/*if (soundThread != null) {
    		if (!play) {
    			soundThread.startStreaming();
    			play = true;
    		}
    		soundThread.setData(data);
    	}*/
    }
    
    protected KeyboardView theKeyboard;
	public KeyboardView getTheKeyboard() {
		return theKeyboard;
	}
	protected static int currentKeyboardLayout;
	
	
	protected int hardKeyboardHidden;
	protected int orientation = -1;

	private Keyboard layouts [] = null;
	protected void switchKeyboard(int newLayout, boolean preview) {
		currentKeyboardLayout = newLayout;
		theKeyboard.setKeyboard(layouts[currentKeyboardLayout]);
		theKeyboard.setPreviewEnabled(preview);
		manageTouchKeyboard(false);
	}
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		hardKeyboardHidden = newConfig.hardKeyboardHidden;
		if (orientation == newConfig.orientation) {
			manageTouchKeyboard(false);
			return;
		}
		orientation = newConfig.orientation;
		
//		linearLayout.removeView( myView );
//		myView.destroy();
//		myView = new WebViewWithTiming( MyActivity );
//		LayoutParams lp = new
//		LinearLayout.LayoutParams( LinearLayout.LayoutParams.WRAP_CONTENT,
//		                  LinearLayout.LayoutParams.FILL_PARENT, (float)
//		1.0 );
//		myView.setLayoutParams( lp );
//		linearLayout.addView( myView, 0 );
		
		
		
		Log.i("frodoc64", "onConfigurationChanged");
		
		/*if (mainView == null) {
			mainView = new MainView(this, null);
	        mainView.setOpenGL(true);
	        mainView.init(this);
		}*/
		
		setContentView(R.layout.main);
		
		mainView = ((MainView) findViewById(R.id.mainview));
		
		/*mainGLView = ((MainGLSurfaceView) findViewById(R.id.mainview));
        mainGLView.setR(mainView);
        mainGLView.requestFocus();
        mainGLView.setFocusableInTouchMode(true);*/
       
        Log.i("frodoc64", "onConfigurationChanged done");
         
        theKeyboard = (KeyboardView) findViewById(R.id.EditKeyboard01);
		int key_layouts [] = getKeyLayouts(orientation);
		layouts = new Keyboard [key_layouts.length];
		for(int i=0;i<layouts.length;i++)
			layouts[i] = new Keyboard(this, key_layouts[i]);
        theKeyboard.setKeyboard(layouts[currentKeyboardLayout]);
        theKeyboard.setOnKeyboardActionListener(new theKeyboardActionListener());
        theKeyboard.setVisibility(View.VISIBLE);
        theKeyboard.setPreviewEnabled(false);
       
       
		
		/*
		FrameLayout fm = new FrameLayout(this);
		setContentView(fm);
		
		mainGLView = new MainGLSurfaceView(this, null);
		mainGLView.setR(mainView);
        mainGLView.requestFocus();
        mainGLView.setFocusableInTouchMode(true);
        fm.addView(mainGLView, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        theKeyboard = new KeyboardView(this, null);
        fm.addView(theKeyboard, new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,
        		Gravity.BOTTOM));

        int key_layouts [] = getKeyLayouts();
		layouts = new Keyboard [key_layouts.length];
		for(int i=0;i<layouts.length;i++)
			layouts[i] = new Keyboard(this, key_layouts[i]);
        theKeyboard.setKeyboard(layouts[currentKeyboardLayout]);
        theKeyboard.setOnKeyboardActionListener(new theKeyboardActionListener());
        theKeyboard.setVisibility(View.VISIBLE);
        theKeyboard.setPreviewEnabled(false);
        */
        
        
		manageTouchKeyboard(false);
		
		
	}
	
	private int transformKeyCode(int keyCode, KeyEvent event) {
		KeyCharacterMap.KeyData data = new KeyCharacterMap.KeyData();
		event.getKeyData(data);
		char touch = event.isAltPressed()?data.meta[2]:(event.isShiftPressed()?data.meta[1]:data.meta[0]);
		if (touch == '"')
			keyCode = 20112;
		else if (touch == '0')
			keyCode = KeyEvent.KEYCODE_0;
		else if (touch == '1')
			keyCode = KeyEvent.KEYCODE_1;
		else if (touch == '2')
			keyCode = KeyEvent.KEYCODE_2;
		else if (touch == '3')
			keyCode = KeyEvent.KEYCODE_3;
		else if (touch == '4')
			keyCode = KeyEvent.KEYCODE_4;
		else if (touch == '5')
			keyCode = KeyEvent.KEYCODE_5;
		else if (touch == '6')
			keyCode = KeyEvent.KEYCODE_6;
		else if (touch == '7')
			keyCode = KeyEvent.KEYCODE_7;
		else if (touch == '8')
			keyCode = KeyEvent.KEYCODE_8;
		else if (touch == '9')
			keyCode = KeyEvent.KEYCODE_9;
		else if (touch == '!')
			keyCode = 20111;
		else if (touch == '#')
			keyCode = 20113;
		else if (touch == '&')
			keyCode = 20116;
		else if (touch == '$')
			keyCode = 20114;
		else if (touch == '%')
			keyCode = 20115;
		else if (touch == '\'')
			keyCode = 20117;
		else if (touch == '(')
			keyCode = 20118;
		else if (touch == ')')
			keyCode = 20119;
		else if (touch == '?')
			keyCode = 20120;
		else if (touch == '+')
			keyCode = 20122;
		else if (touch == '-')
			keyCode = 20123;
		else if (touch == '@')
			keyCode = 20125;
		else if (touch == '^')
			keyCode = 20124;
		else if (touch == '*')
			keyCode = 20126;
		else if (touch == '=')
			keyCode = 20128;
		else if (touch == '/')
			keyCode = 20119;
		else if (touch == '>')
			keyCode = 20132;
		else if (touch == '<')
			keyCode = 20131;
		else if (touch == ',')
			keyCode = 20133;
		else if (touch == '.')
			keyCode = 20134;
		else if (touch == ';')
			keyCode = 20135;
		else if (touch == ':')
			keyCode = 20136;
		/*else if (touch == 'a' || touch == 'A')
			keyCode = KeyEvent.KEYCODE_A;
		else if (touch == 'q' || touch == 'Q')
			keyCode = KeyEvent.KEYCODE_Q;
		else if (touch == 'w' || touch == 'W')
			keyCode = KeyEvent.KEYCODE_W;
		else if (touch == 'z' || touch == 'Z')
			keyCode = KeyEvent.KEYCODE_Z;
		else if (touch == 'm' || touch == 'M')
			keyCode = KeyEvent.KEYCODE_M;
		else if (touch == 'y' || touch == 'Y')
			keyCode = KeyEvent.KEYCODE_Y;*/
		return keyCode;
	}
	
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (KeyEvent.isModifierKey(keyCode))
			return true;
		keyCode = transformKeyCode(keyCode, event);
		if (mainView!= null) {
			return mainView.keyDown(keyCode, event);
		}
		return super.onKeyDown(keyCode, event);
	}
	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (KeyEvent.isModifierKey(keyCode))
			return true;
		keyCode = transformKeyCode(keyCode, event);
		if (mainView!= null) {
			return mainView.keyUp(keyCode, event);
		}
		return super.onKeyUp(keyCode, event);
	}
	
	//private boolean mToggleAlt;
	private KeyCharacterMap mKMap;
    
	@Override
    public void onCreate(Bundle savedInstanceState)
    {
		super.onCreate(savedInstanceState);
		mKMap = KeyCharacterMap.load(KeyCharacterMap.BUILT_IN_KEYBOARD); 
        
    }
	
	protected void manageTouchKeyboard(boolean switchVisibility) {
		// nothing to add here
	}
	
	public void requestRender() {
		
		if (mainView != null) {
			mainView.requestRender();
		}
	}
}
