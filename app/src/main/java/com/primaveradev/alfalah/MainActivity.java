package com.primaveradev.alfalah;

import android.content.res.AssetManager;
import android.media.MediaPlayer;
import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.MotionEvent;

import java.io.IOException;

import androidx.appcompat.app.AppCompatActivity;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("alfalah");
    }

    private static MediaPlayer sMediaPlayer;

    public static void playAyah(int surah, int ayah) {
        if (sMediaPlayer != null) {
            sMediaPlayer.stop();
            sMediaPlayer.release();
            sMediaPlayer = null;
        }
        String url = String.format(
            "https://cdn.islamic.network/quran/audio/128/ar.alafasy/%03d%03d.mp3",
            surah, ayah);
        sMediaPlayer = new MediaPlayer();
        try {
            sMediaPlayer.setDataSource(url);
            sMediaPlayer.prepareAsync();
            sMediaPlayer.setOnPreparedListener(mp -> mp.start());
            sMediaPlayer.setOnCompletionListener(mp -> {
                mp.release();
                sMediaPlayer = null;
            });
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public native void nativeOnSurfaceCreated(AssetManager assetManager, float density);
    public native void nativeOnSurfaceChanged(int width, int height);
    public native void nativeOnDrawFrame();
    public native void nativeSetStatusBarHeight(float heightDp);
    public native void nativeOnTouch(float x, float y, boolean down);

    private GLSurfaceView glView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        glView = new GLSurfaceView(this);

        // Request OpenGL ES 3.0
        glView.setEGLContextClientVersion(3);

        glView.setRenderer(new GLSurfaceView.Renderer() {
            @Override
            public void onSurfaceCreated(GL10 gl, EGLConfig config) {
                float density = getResources().getDisplayMetrics().density;
                nativeOnSurfaceCreated(getAssets(), density);
                int statusBarId = getResources().getIdentifier("status_bar_height", "dimen", "android");
                int px = statusBarId > 0 ? getResources().getDimensionPixelSize(statusBarId) : 0;
                nativeSetStatusBarHeight(px / density);
            }

            @Override
            public void onSurfaceChanged(GL10 gl, int width, int height) {
                nativeOnSurfaceChanged(width, height);
            }

            @Override
            public void onDrawFrame(GL10 gl) {
                nativeOnDrawFrame();
            }
        });

        glView.setOnTouchListener((v, event) -> {
            int action = event.getActionMasked();
            nativeOnTouch(event.getX(), event.getY(),
                    action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_MOVE);
            return true;
        });

        setContentView(glView);
    }

    @Override
    protected void onPause() {
        super.onPause();
        glView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        glView.onResume();
    }
}