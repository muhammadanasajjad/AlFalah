package com.primaveradev.alfalah;

import android.content.res.AssetManager;
import android.media.MediaPlayer;
import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Base64;
import android.view.MotionEvent;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;

import androidx.appcompat.app.AppCompatActivity;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("alfalah");
    }

    private static MediaPlayer sMediaPlayer;
    private static final Object sPlayerLock = new Object();

    // Quran Foundation API — production
    private static final String CLIENT_ID = "2ca5c4ac-4dad-4186-af9a-3f736614a241";
    private static final String CLIENT_SECRET = "cBvlkVG.cLfiJDFa47RVMj5I9z";
    private static final String AUTH_URL = "https://oauth2.quran.foundation/oauth2/token";
    private static final String API_BASE = "https://apis.quran.foundation";
    private static final int RECITATION_ID = 7; // Mishari Rashid al-Afasy (Murattal)

    private static String sAccessToken = null;
    private static long sTokenExpiry = 0;

    private static synchronized String getAccessToken() {
        long now = System.currentTimeMillis() / 1000;
        if (sAccessToken != null && now < sTokenExpiry - 60) return sAccessToken;

        try {
            URL url = new URL(AUTH_URL);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
            String creds = CLIENT_ID + ":" + CLIENT_SECRET;
            String auth = "Basic " + Base64.encodeToString(creds.getBytes(), Base64.NO_WRAP);
            conn.setRequestProperty("Authorization", auth);
            conn.setDoOutput(true);

            String body = "grant_type=client_credentials&scope=content";
            try (OutputStream os = conn.getOutputStream()) {
                os.write(body.getBytes(StandardCharsets.UTF_8));
            }

            int code = conn.getResponseCode();
            if (code != 200) return null;

            BufferedReader br = new BufferedReader(
                new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) sb.append(line);
            br.close();

            JSONObject json = new JSONObject(sb.toString());
            sAccessToken = json.getString("access_token");
            sTokenExpiry = now + json.getInt("expires_in");
            return sAccessToken;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    private static String fetchAudioUrl(int surah, int ayah) {
        String token = getAccessToken();
        if (token == null) return null;

        String endpoint = API_BASE + "/content/api/v4/chapters/" + surah
            + "/recitations/" + RECITATION_ID + "?per_page=50";

        try {
            URL url = new URL(endpoint);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestProperty("x-auth-token", token);
            conn.setRequestProperty("x-client-id", CLIENT_ID);

            int code = conn.getResponseCode();
            if (code != 200) return null;

            BufferedReader br = new BufferedReader(
                new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) sb.append(line);
            br.close();

            JSONObject json = new JSONObject(sb.toString());
            JSONArray files = json.getJSONArray("audio_files");
            String targetKey = surah + ":" + ayah;
            for (int i = 0; i < files.length(); i++) {
                JSONObject f = files.getJSONObject(i);
                if (f.getString("verse_key").equals(targetKey)) {
                    return f.getString("url");
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public static void playAyah(int surah, int ayah) {
        synchronized (sPlayerLock) {
            if (sMediaPlayer != null) {
                sMediaPlayer.stop();
                sMediaPlayer.release();
                sMediaPlayer = null;
            }
        }

        new Thread(() -> {
            String path = fetchAudioUrl(surah, ayah);
            if (path == null) {
                path = String.format(
                    "https://cdn.islamic.network/quran/audio/128/ar.alafasy/%03d%03d.mp3",
                    surah, ayah);
            } else if (!path.startsWith("http")) {
                path = "https://cdn.islamic.network/quran/audio/128/ar.alafasy/"
                    + path.replace("Alafasy/mp3/", "");
            }
            final String url = path;
            synchronized (sPlayerLock) {
                sMediaPlayer = new MediaPlayer();
                try {
                    sMediaPlayer.setDataSource(url);
                    sMediaPlayer.prepareAsync();
                    sMediaPlayer.setOnPreparedListener(mp -> mp.start());
                    sMediaPlayer.setOnCompletionListener(mp -> {
                        synchronized (sPlayerLock) {
                            mp.release();
                            sMediaPlayer = null;
                        }
                    });
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }).start();
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