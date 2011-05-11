package org.ab.nativelayer;

import java.nio.ByteBuffer;
import java.nio.ShortBuffer;
import java.util.List;
import java.util.Vector;


import org.ab.c64.FrodoC64;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MainView extends SurfaceView implements SurfaceHolder.Callback {
	
	private static List<Integer> queuedCodes;
	public String id;
	private static ByteBuffer drawCanvasBuffer;
	static ShortBuffer drawCanvasBufferAsShort;
	private int bufferWidth;
	private int bufferHeight;
	
	private int width;
	private int height;
	
	public void startEmulation() {
		((MainActivity) getContext()).startEmulation();
	}
	
	protected SurfaceHolder mSurfaceHolder;
	protected static Bitmap mainScreen;
	protected float scaleX;
	protected float scaleY;
	protected boolean coordsChanged;
	Matrix matrixScreen;
	
	public void initMatrix() {
		if (width < height) {
			scaleY = scaleX;
		} else {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getContext());
		 String scale = prefs.getString("scale", "stretched");
		 pixels = 0;
		 pixelsH = 0;
		 if ("scaled".equals(scale)) {
			 scaleX = scaleY;
			 pixels = (int) ((width - bufferWidth*scaleX) / 2);
		 } else if ("1x".equals(scale)) {
			 scaleX = 1.0f;
			 scaleY = 1.0f;
			 pixels = (int) ((width - bufferWidth*scaleX) / 2);
			 pixelsH = (int) ((height - bufferHeight*scaleX) / 2);
		 } else if ("2x".equals(scale)) {
			 scaleX = 2.0f;
			 scaleY = 2.0f;
			 pixels = (int) ((width - bufferWidth*scaleX) / 2);
			 pixelsH = (int) ((height - bufferHeight*scaleX) / 2);
		 }
		}
		 matrixScreen = new Matrix();
		 matrixScreen.setScale(scaleX, scaleY);
		 matrixScreen.postTranslate(pixels, pixelsH);
   	 
	}
	
	public void surfaceCreated(SurfaceHolder holder) {
		Log.i(id, "Surface created");
		bufferWidth = ((MainActivity) getContext()).getWidth();
		bufferHeight = ((MainActivity) getContext()).getHeight();
		//if (mainScreen == null) {
			mainScreen = Bitmap.createBitmap(bufferWidth,bufferHeight, Bitmap.Config.RGB_565);
			Log.i(id, "Create screen buffers");
			drawCanvasBuffer = ByteBuffer.allocateDirect(bufferWidth * bufferHeight * 2);
			drawCanvasBufferAsShort = drawCanvasBuffer.asShortBuffer();
			((FrodoC64) getContext()).initScreenData(drawCanvasBufferAsShort, bufferWidth, bufferHeight, 0, 0, ((FrodoC64) getContext()).isBorder()?1:0);
		//}
			
			
		if (MainActivity.emulationThread == null) {
			MainActivity.emulationThread = new Thread() {
				public void run() {
					Log.i(id, "Emulation thread started");
					startEmulation();
					Log.i(id, "Emulation thread exited");
				}
			};
			MainActivity.emulationThread.start();
		}
		
		//updater = new ScreenUpdater(this);
	}
	
	public void surfaceDestroyed(SurfaceHolder holder) {
		Log.i(id, "Surface destroyed");
		
		if (updater != null)
			updater.stop();
	}
	

	public void emulatorReady() {
		
	}
	
	
	
	public void stop() {
		Log.i(id, "Stopping..");
		if (MainActivity.emulationThread != null)
			MainActivity.emulationThread.interrupt();
		MainActivity.emulationThread = null;
	}
	
	long t = System.currentTimeMillis();
	int frames;
	public void checkFPS() {

        
        if (frames % 20 == 0) {
        	long t2 = System.currentTimeMillis();
        	Log.i("uae", "FPS: " + 20000 / (t2 - t));
        	t = t2;
        }
        frames++;
        
	}
	
	
	public void requestRender() {
		
	       
		if (updater != null) {
			updater.render();
		} else {
			// checkFPS();
				Canvas c = null;
		        try {
		            c = mSurfaceHolder.lockCanvas(null);
		            synchronized (mSurfaceHolder) {
		            	 mainScreen.copyPixelsFromBuffer(drawCanvasBufferAsShort);
		            	 if (c != null) {
		            		
		            	 c.drawBitmap(mainScreen, matrixScreen, null);
		            	 if (((MainActivity) getContext()).vKeyPad != null)
		         			((MainActivity) getContext()).vKeyPad.draw(c);
		            	}
		            	 
		            }
		        } finally {
		            // do this in a finally so that if an exception is thrown
		            // during the above, we don't leave the Surface in an
		            // inconsistent state
		            if (c != null) {
		                mSurfaceHolder.unlockCanvasAndPost(c);
		            }
		        }
			}
		}
	
	ScreenUpdater updater;
	
	public MainView(Context context, AttributeSet attrs) {
		super(context, attrs);
		
		setFocusable(true);
        setFocusableInTouchMode(true);
        mSurfaceHolder = getHolder();
        mSurfaceHolder.addCallback(this);
		id = ((MainActivity) getContext()).getID();
		if (queuedCodes == null)
		queuedCodes = new Vector<Integer>();
		
	}
	
	protected synchronized void key(int real_keycode, boolean down) {
		
		if (real_keycode >> 16 > 0) {
			// two codes!
			int code1 = real_keycode >> 16; 
			int code2 = real_keycode & 0x0000ffff;
			if (down) {
				((FrodoC64) getContext()).sendKey(-code1-2);
				((FrodoC64) getContext()).sendKey(-code2-2);
			} else {
				((FrodoC64) getContext()).sendKey(code2);
				((FrodoC64) getContext()).sendKey(code1);
			}
		} else
			((FrodoC64) getContext()).sendKey(down?-real_keycode-2:real_keycode);
	}
	
	public int waitEvent() {
		
		if (queuedCodes.size() == 0)
			return -1;
		else {
			int c = queuedCodes.remove(0);
			//Log.i("frodoc64", "c:" + c);
			return c;
		}
    }
	
	public boolean keyDown(int keyCode, KeyEvent event) {
		boolean b = actionKey(true, keyCode);
		sleepAfterInputEvent();
		return b;
	}
	
	
	public boolean keyUp(int keyCode, KeyEvent event) {
		boolean b = actionKey(false, keyCode);
		sleepAfterInputEvent();
		return b;
	}
	/*
	public boolean touchEvent(MotionEvent event) {
		if (event.getAction() == MotionEvent.ACTION_DOWN) {
			((MainActivity) getContext()).manageTouchKeyboard(true);
			return true;
		}
		return false;
	}
	*/
	protected void sleepAfterInputEvent() {
		/*try {
			Thread.sleep(16);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}*/
	}
	
	
	
	public boolean actionKey(boolean down, int keycode) {
		
		
		int real = ((MainActivity) getContext()).getRealKeycode(keycode, down);
		if (real == Integer.MAX_VALUE) {
			return true;
		} else if (real == Integer.MIN_VALUE) {
			return false;
		} else {
			key(real, down);
			return true;
		} 
		
	}
	

	public boolean isOpenGL() {
		return false;
	}

	public void setOpenGL(boolean openGL) {
		
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		coordsChanged = true;
		this.width = width;
		this.height = height;
		scaleX = (float) (width-pixels)/bufferWidth;
		scaleY = (float) height/bufferHeight;
		
		 initMatrix();
		 
		 if (updater != null)
				updater.start();
		if (((MainActivity) getContext()).vKeyPad != null)
			((MainActivity) getContext()).vKeyPad.resize(width, height);
		
		
	}
	
	private int pixels;
	private int pixelsH;
	public void shiftImage(int leftDPIs) {
		if (leftDPIs > 0) {
			DisplayMetrics metrics = new DisplayMetrics();
	        ((MainActivity) getContext()).getWindowManager().getDefaultDisplay().getMetrics(metrics);
	        pixels = (int) (leftDPIs * metrics.density + 0.5f);
	        Log.i("frodo64", "pixels: " + pixels);
	        scaleX = (float) (width-pixels)/bufferWidth;
			scaleY = (float) height/bufferHeight;
			coordsChanged = true;
			
		} else {
			pixels = 0;
			scaleX = (float) width/bufferWidth;
			scaleY = (float) height/bufferHeight;
			coordsChanged = true;
	
		}
		
		
		initMatrix();
	}

	public void init(Context context) {
		
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		/*if (event.getAction() == MotionEvent.ACTION_UP) {
			return true;
		}
		return false;*/
		if (((MainActivity) getContext()).vKeyPad != null) {
			boolean b = ((MainActivity) getContext()).vKeyPad.onTouch(event, false);
			
			return b;
		}
		
		
		return false;
	}
	
	
}
