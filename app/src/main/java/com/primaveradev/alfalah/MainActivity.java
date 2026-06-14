package com.primaveradev.alfalah;

import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("alfalah");
    }

    public native void nativeOnSurfaceCreated();
    public native void nativeOnSurfaceChanged(int width, int height);
    public native void nativeOnDrawFrame();

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
                nativeOnSurfaceCreated();
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