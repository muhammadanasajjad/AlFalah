package com.primaveradev.alfalah;

import android.content.res.AssetManager;
import android.media.MediaPlayer;
import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Base64;
import android.util.Log;
import android.view.MotionEvent;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
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

    private static final String TAG = "AlFalah";

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
        if (sAccessToken != null && now < sTokenExpiry - 60) {
            Log.i(TAG, "getAccessToken: using cached token (expires in " + (sTokenExpiry - now) + "s)");
            return sAccessToken;
        }

        Log.i(TAG, "getAccessToken: requesting new token from " + AUTH_URL);
        try {
            URL url = new URL(AUTH_URL);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
            String creds = CLIENT_ID + ":" + CLIENT_SECRET;
            String auth = "Basic " + Base64.encodeToString(creds.getBytes(), Base64.NO_WRAP);
            conn.setRequestProperty("Authorization", auth);
            conn.setDoOutput(true);
            conn.setConnectTimeout(10000);
            conn.setReadTimeout(10000);

            String body = "grant_type=client_credentials&scope=content";
            try (OutputStream os = conn.getOutputStream()) {
                os.write(body.getBytes(StandardCharsets.UTF_8));
            }

            int code = conn.getResponseCode();
            Log.i(TAG, "getAccessToken: response code " + code);
            if (code != 200) {
                Log.w(TAG, "getAccessToken: failed with code " + code);
                return null;
            }

            BufferedReader br = new BufferedReader(
                new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) sb.append(line);
            br.close();

            JSONObject json = new JSONObject(sb.toString());
            sAccessToken = json.getString("access_token");
            sTokenExpiry = now + json.getInt("expires_in");
            Log.i(TAG, "getAccessToken: obtained token, expires in " + json.getInt("expires_in") + "s");
            return sAccessToken;
        } catch (Exception e) {
            Log.e(TAG, "getAccessToken: exception", e);
            return null;
        }
    }

    private static String fetchAudioUrl(int surah, int ayah) {
        Log.i(TAG, "fetchAudioUrl: surah=" + surah + " ayah=" + ayah);
        String token = getAccessToken();
        if (token == null) {
            Log.w(TAG, "fetchAudioUrl: no access token, falling back");
            return null;
        }

        String endpoint = API_BASE + "/content/api/v4/chapters/" + surah
            + "/recitations/" + RECITATION_ID + "?per_page=50";
        Log.i(TAG, "fetchAudioUrl: hitting " + endpoint);

        try {
            URL url = new URL(endpoint);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestProperty("x-auth-token", token);
            conn.setRequestProperty("x-client-id", CLIENT_ID);
            conn.setConnectTimeout(10000);
            conn.setReadTimeout(10000);

            int code = conn.getResponseCode();
            Log.i(TAG, "fetchAudioUrl: response code " + code);
            if (code != 200) {
                Log.w(TAG, "fetchAudioUrl: API returned " + code);
                return null;
            }

            BufferedReader br = new BufferedReader(
                new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) sb.append(line);
            br.close();

            JSONObject json = new JSONObject(sb.toString());
            JSONArray files = json.getJSONArray("audio_files");
            String targetKey = surah + ":" + ayah;
            Log.i(TAG, "fetchAudioUrl: got " + files.length() + " audio files, looking for " + targetKey);
            for (int i = 0; i < files.length(); i++) {
                JSONObject f = files.getJSONObject(i);
                String vk = f.getString("verse_key");
                if (vk.equals(targetKey)) {
                    String urlStr = f.getString("url");
                    Log.i(TAG, "fetchAudioUrl: found URL: " + urlStr);
                    return urlStr;
                }
            }
            Log.w(TAG, "fetchAudioUrl: verse_key " + targetKey + " not found in response");
        } catch (Exception e) {
            Log.e(TAG, "fetchAudioUrl: exception", e);
        }
        return null;
    }

    public static void playAyah(int surah, int ayah) {
        Log.i(TAG, "playAyah(" + surah + ", " + ayah + ") called from JNI");
        synchronized (sPlayerLock) {
            if (sMediaPlayer != null) {
                Log.i(TAG, "playAyah: stopping previous playback");
                sMediaPlayer.stop();
                sMediaPlayer.release();
                sMediaPlayer = null;
            }
        }

        new Thread(() -> {
            Log.i(TAG, "playAyah: background thread started");
            String path = fetchAudioUrl(surah, ayah);
            if (path == null) {
                Log.i(TAG, "playAyah: API returned null, using CDN fallback");
                path = String.format(
                    "https://cdn.islamic.network/quran/audio/128/ar.alafasy/%03d%03d.mp3",
                    surah, ayah);
            } else if (!path.startsWith("http")) {
                String filename = path.substring(path.lastIndexOf('/') + 1);
                path = "https://cdn.islamic.network/quran/audio/128/ar.alafasy/" + filename;
                Log.i(TAG, "playAyah: resolved relative URL to " + path);
            } else {
                Log.i(TAG, "playAyah: using absolute URL from API: " + path);
            }
            final String url = path;
            Log.i(TAG, "playAyah: final audio URL: " + url);
            synchronized (sPlayerLock) {
                sMediaPlayer = new MediaPlayer();
                try {
                    sMediaPlayer.setDataSource(url);
                    sMediaPlayer.prepareAsync();
                    Log.i(TAG, "playAyah: MediaPlayer created, preparing async");
                    sMediaPlayer.setOnPreparedListener(mp -> {
                        Log.i(TAG, "playAyah: prepared, starting playback");
                        mp.start();
                    });
                    sMediaPlayer.setOnCompletionListener(mp -> {
                        Log.i(TAG, "playAyah: playback completed");
                        synchronized (sPlayerLock) {
                            mp.release();
                            sMediaPlayer = null;
                        }
                    });
                    sMediaPlayer.setOnErrorListener((mp, what, extra) -> {
                        Log.e(TAG, "playAyah: MediaPlayer error what=" + what + " extra=" + extra);
                        synchronized (sPlayerLock) {
                            mp.release();
                            sMediaPlayer = null;
                        }
                        return true;
                    });
                } catch (IOException e) {
                    Log.e(TAG, "playAyah: setDataSource exception", e);
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