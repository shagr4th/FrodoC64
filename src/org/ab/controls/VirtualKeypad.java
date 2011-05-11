package org.ab.controls;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.drawable.BitmapDrawable;
import android.preference.PreferenceManager;
import android.view.MotionEvent;
import android.view.View;
import java.util.ArrayList;

public class VirtualKeypad {

	public static int LEFT = 1;
	public static int RIGHT = 2;
	public static int UP = 4;
	public static int DOWN = 8;
	public static int BUTTON = 16;

	private static final float DPAD_DEADZONE_VALUES[] = {
		0.1f, 0.14f, 0.1667f, 0.2f, 0.25f,
	};

	private Context context;
	private View view;
	private float scaleX;
	private float scaleY;
	private int transparency;

	private GameKeyListener gameKeyListener;
	private int keyStates;
	private float dpadDeadZone = DPAD_DEADZONE_VALUES[2];

	private ArrayList<Control> controls = new ArrayList<Control>();
	private Control dpad;
	private Control buttons;

	
	public VirtualKeypad(View v, GameKeyListener l, int dpadRes, int buttonsRes) {
		view = v;
		context = view.getContext();
		gameKeyListener = l;

		dpad = createControl(dpadRes);
		buttons = createControl(buttonsRes);
	}

	public final int getKeyStates() {
		return keyStates;
	}

	public void reset() {
		keyStates = 0;
	}

	public final void destroy() {
	}

	public final void resize(int w, int h) {
		SharedPreferences prefs = PreferenceManager.
				getDefaultSharedPreferences(context);
		
		//int value = prefs.getInt("dpadDeadZone", 2);
		//value = (value < 0 ? 0 : (value > 4 ? 4 : value));
		dpadDeadZone = DPAD_DEADZONE_VALUES[2];
		
		dpad.hide(prefs.getBoolean("hideDpad", false));
		buttons.hide(!Wrapper.supportsMultitouch(context) || prefs.getBoolean("hideButtons", false));

		scaleX = (float) w / view.getWidth();
		scaleY = (float) h / view.getHeight();

		float controlScale = getControlScale(prefs);
		float sx = scaleX * controlScale;
		float sy = scaleY * controlScale;
		Resources res = context.getResources();
		for (Control c : controls)
			c.load(res, sx, sy);

		reposition(w, h, prefs);

		transparency = prefs.getInt("vkeypadTransparency", 50);
	}

	public void draw(Canvas canvas) {
		Paint paint = new Paint();
		paint.setAlpha(transparency * 2 + 30);

		for (Control c : controls)
			c.draw(canvas, paint);
	}

	private static float getControlScale(SharedPreferences prefs) {
		String value = prefs.getString("vkeypadSize", null);
		if ("small".equals(value))
			return 1.0f;
		if ("large".equals(value))
			return 1.33333f;
		return 1.2f;
	}

	private Control createControl(int resId) {
		Control c = new Control(resId);
		controls.add(c);
		return c;
	}

	private void makeBottomBottom(int w, int h) {
		if (dpad.getWidth() + buttons.getWidth() > w) {
			makeBottomTop(w, h);
			return;
		}

		dpad.move(0, h - dpad.getHeight());
		buttons.move(w - buttons.getWidth(), h - buttons.getHeight());
		
	}

	private void makeTopTop(int w, int h) {
		if (dpad.getWidth() + buttons.getWidth() > w) {
			makeBottomTop(w, h);
			return;
		}

		dpad.move(0, 0);
		buttons.move(w - buttons.getWidth(), 0);

		
	}

	private void makeTopBottom(int w, int h) {
		dpad.move(0, 0);
		buttons.move(w - buttons.getWidth(), h - buttons.getHeight());

		
	}

	private void makeBottomTop(int w, int h) {
		dpad.move(0, h - dpad.getHeight());
		buttons.move(w - buttons.getWidth(), 0);

		
	}

	private void reposition(int w, int h, SharedPreferences prefs) {
		String layout = prefs.getString("vkeypadLayout", "bottom_bottom");

		if ("top_bottom".equals(layout))
			makeTopBottom(w, h);
		else if ("bottom_top".equals(layout))
			makeBottomTop(w, h);
		else if ("top_top".equals(layout))
			makeTopTop(w, h);
		else
			makeBottomBottom(w, h);
	}

	private void setKeyStates(int newStates) {
		if (keyStates == newStates)
			return;

		keyStates = newStates;
		gameKeyListener.onGameKeyChanged(keyStates);
	}

	private int getDpadStates(float x, float y) {
		
		final float cx = 0.5f;
		final float cy = 0.5f;
		int states = 0;

		if (x < cx - dpadDeadZone)
			states |= LEFT;
		else if (x > cx + dpadDeadZone)
			states |= RIGHT;
		if (y < cy - dpadDeadZone)
			states |= UP;
		else if (y > cy + dpadDeadZone)
			states |= DOWN;

		return states;
	}

	private int getButtonsStates(float x, float y, float size) {
		int states = BUTTON;
		return states;
	}

	private float getEventX(MotionEvent event, int index, boolean flip) {
		float x = Wrapper.MotionEvent_getX(event, index);
		if (flip)
			x = view.getWidth() - x;
		return (x * scaleX);
	}

	private float getEventY(MotionEvent event, int index, boolean flip) {
		float y = Wrapper.MotionEvent_getY(event, index);
		if (flip)
			y = view.getHeight() - y;
		return y * scaleY;
	}

	private Control findControl(float x, float y) {
		for (Control c : controls) {
			if (c.hitTest(x, y))
				return c;
		}
		return null;
	}

	private int getControlStates(Control c, float x, float y, float size) {
		x = (x - c.getX()) / c.getWidth();
		y = (y - c.getY()) / c.getHeight();

		if (c == dpad)
			return getDpadStates(x, y);
		if (c == buttons)
			return getButtonsStates(x, y, size);
		
		return 0;
	}

	public boolean onTouch(MotionEvent event, boolean flip) {
		int action = event.getAction();
		
		switch (action & MotionEvent.ACTION_MASK) {
		case MotionEvent.ACTION_UP:
		case MotionEvent.ACTION_CANCEL:
			setKeyStates(0);
			return true;

		case MotionEvent.ACTION_DOWN:
		case MotionEvent.ACTION_POINTER_DOWN:
		case MotionEvent.ACTION_MOVE:
		case MotionEvent.ACTION_OUTSIDE:
			break;
		default:
			return false;
		}

		int states = 0;
		int n = Wrapper.MotionEvent_getPointerCount(event);
		for (int i = 0; i < n; i++) {
			float x = getEventX(event, i, flip);
			float y = getEventY(event, i, flip);
			Control c = findControl(x, y);
			if (c != null) {
				states |= getControlStates(c, x, y,
						Wrapper.MotionEvent_getSize(event, i));
			}
		}
		setKeyStates(states);
		return true;
	}


	private static class Control {
		private int resId;
		private boolean hidden;
		private boolean disabled;
		private Bitmap bitmap;
		private RectF bounds = new RectF();

		Control(int r) {
			resId = r;
		}

		final float getX() {
			return bounds.left;
		}

		final float getY() {
			return bounds.top;
		}

		final int getWidth() {
			return bitmap.getWidth();
		}

		final int getHeight() {
			return bitmap.getHeight();
		}

		final boolean isEnabled() {
			return !disabled;
		}

		final void hide(boolean b) {
			hidden = b;
		}

		final void disable(boolean b) {
			disabled = b;
		}

		final boolean hitTest(float x, float y) {
			return bounds.contains(x, y);
		}

		final void move(float x, float y) {
			bounds.set(x, y, x + bitmap.getWidth(), y + bitmap.getHeight());
		}

		final void load(Resources res, float sx, float sy) {
			bitmap = ((BitmapDrawable) res.getDrawable(resId)).getBitmap();
			bitmap = Bitmap.createScaledBitmap(bitmap,
					(int) (sx * bitmap.getWidth()),
					(int) (sy * bitmap.getHeight()), true);
		}

		final void reload(Resources res, int id) {
			int w = bitmap.getWidth();
			int h = bitmap.getHeight();
			bitmap = ((BitmapDrawable) res.getDrawable(id)).getBitmap();
			bitmap = Bitmap.createScaledBitmap(bitmap, w, h, true);
		}

		final void draw(Canvas canvas, Paint paint) {
			if (!hidden && !disabled && bitmap != null)
				canvas.drawBitmap(bitmap, bounds.left, bounds.top, paint);
		}
	}
}
