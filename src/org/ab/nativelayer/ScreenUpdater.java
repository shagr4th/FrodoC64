package org.ab.nativelayer;

import android.graphics.Canvas;
import android.util.Log;

public class ScreenUpdater implements Runnable {
	
	MainView view;
	Thread thread;
	boolean running = false;
	
	public ScreenUpdater(MainView view) {
		this.view = view;
	}
	
	public void start() {
		running = true;
		thread = new Thread(this);
		thread.start();
	}
	
	public void stop() {
		running = false;
		render();
	}
	
	long t = System.currentTimeMillis();
	int frames;
	public void checkFPS() {

        frames++;
        
        if (frames % 20 == 0) {
        	long t2 = System.currentTimeMillis();
        	Log.i("uae", "UFPS: " + 20000 / (t2 - t));
        	t = t2;
        }
        
	}
	
	public void render() {
		synchronized (this) {
			notify();
		}
	}


	public void run() {
		while (running) {
			
			
			 checkFPS();
			Canvas c = null;
	        try {
	            c = view.mSurfaceHolder.lockCanvas();
	            
	          
	            if (c != null) {
	            	MainView.mainScreen.copyPixelsFromBuffer(MainView.drawCanvasBufferAsShort);
	            	
		            
		            
		            c.drawBitmap(MainView.mainScreen, view.matrixScreen, null);
		            if (((MainActivity) view.getContext()).vKeyPad != null)
		            	((MainActivity) view.getContext()).vKeyPad.draw(c);
	            }
           	
	        } catch (Exception e) {
	        	e.printStackTrace();
			} finally {
	            if (c != null) {
	            	view.mSurfaceHolder.unlockCanvasAndPost(c);
	            }
	        }
	        
			
			  try {
					synchronized(this) {
						wait();
					}
				} catch (InterruptedException e) {
					break;
				}
				
	   
		}
	}

}
