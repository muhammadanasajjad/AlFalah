package com.primaveradev.alfalah;

import android.opengl.GLSurfaceView;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("alfalah");
    }

    private GLSurfaceView glView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        glView = new GLSurfaceView(this);

        glView.setEGLContextClientVersion(3);

        glView.setRenderer(new GLSurfaceView.Renderer() {

            @Override
            public void onSurfaceCreated(
                    javax.microedition.khronos.opengles.GL10 gl,
                    javax.microedition.khronos.egl.EGLConfig config) {
            }

            @Override
            public void onSurfaceChanged(
                    javax.microedition.khronos.opengles.GL10 gl,
                    int width,
                    int height) {
            }

            @Override
            public void onDrawFrame(
                    javax.microedition.khronos.opengles.GL10 gl) {

                android.opengl.GLES30.glClearColor(
                        0.0f,
                        0.0f,
                        1.0f,
                        1.0f);

                android.opengl.GLES30.glClear(
                        android.opengl.GLES30.GL_COLOR_BUFFER_BIT);
            }
        });

        setContentView(glView);
    }
}