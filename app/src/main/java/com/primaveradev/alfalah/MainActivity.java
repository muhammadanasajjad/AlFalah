package com.primaveradev.alfalah;

import android.content.Context;
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
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
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

    // Quran Foundation API — pre-production (production returned 404 for recitations)
    private static final String CLIENT_ID = "07aa145f-ab4e-453f-95fa-65c75e2a7e0d";
    private static final String CLIENT_SECRET = "yyttJn5e-N7fpAPUCtnHm4rdos";
    private static final String AUTH_URL = "https://prelive-oauth2.quran.foundation/oauth2/token";
    private static final String API_BASE = "https://apis-prelive.quran.foundation";
    private static final String CDN_BASE = "https://verses.quran.foundation";

    private static Context sContext = null;

    private static class ReciterInfo {
        final int apiId;
        final String displayName;
        final String cdnSlug;
        ReciterInfo(int id, String name, String slug) {
            this.apiId = id;
            this.displayName = name;
            this.cdnSlug = slug;
        }
    }

    private static final ReciterInfo[] sReciters = {
        new ReciterInfo(7,  "Al-Afasy",              "Alafasy/mp3"),
        new ReciterInfo(1,  "AbdulBaset (Mujawwad)", "AbdulBasetAbdulSamad/mp3"),
    };
    private static int sCurrentReciterIndex = 0;

    private static String sAccessToken = null;
    private static long sTokenExpiry = 0;

    // ── JNI-accessible reciter methods ──

    public static int getReciterCount() {
        return sReciters.length;
    }

    public static String getReciterName(int index) {
        if (index < 0 || index >= sReciters.length) return "";
        return sReciters[index].displayName;
    }

    public static void setReciter(int index) {
        if (index >= 0 && index < sReciters.length) {
            sCurrentReciterIndex = index;
            Log.i(TAG, "setReciter: switched to " + sReciters[index].displayName);
        }
    }

    // ── Token management ──

    private static synchronized String getAccessToken() {
        long now = System.currentTimeMillis() / 1000;
        if (sAccessToken != null && now < sTokenExpiry - 60) {
            return sAccessToken;
        }

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
            return sAccessToken;
        } catch (Exception e) {
            Log.e(TAG, "getAccessToken: exception", e);
            return null;
        }
    }

    // ── URL resolution ──

    private static String fetchAudioUrl(int surah, int ayah, int recitationId) {
        String token = getAccessToken();
        if (token == null) return null;

        String endpoint = API_BASE + "/content/api/v4/chapters/" + surah
            + "/recitations/" + recitationId + "?per_page=50";

        try {
            URL url = new URL(endpoint);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestProperty("x-auth-token", token);
            conn.setRequestProperty("x-client-id", CLIENT_ID);
            conn.setConnectTimeout(10000);
            conn.setReadTimeout(10000);

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
                String vk = f.getString("verse_key");
                if (vk.equals(targetKey)) {
                    return f.getString("url");
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "fetchAudioUrl: exception", e);
        }
        return null;
    }

    // ── Local cache ──

    private static String getLocalAudioPath(int surah, int ayah) {
        if (sContext == null) return null;
        File dir = new File(sContext.getFilesDir(),
            "audio/" + sReciters[sCurrentReciterIndex].apiId);
        dir.mkdirs();
        return new File(dir, String.format("%03d%03d.mp3", surah, ayah)).getAbsolutePath();
    }

    private static boolean downloadToFile(String urlStr, String destPath) {
        try {
            URL url = new URL(urlStr);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setConnectTimeout(30000);
            conn.setReadTimeout(30000);

            int code = conn.getResponseCode();
            if (code != 200) {
                Log.w(TAG, "downloadToFile: server returned " + code);
                return false;
            }

            File dest = new File(destPath);
            File parent = dest.getParentFile();
            if (parent != null) parent.mkdirs();

            try (InputStream is = conn.getInputStream();
                 FileOutputStream fos = new FileOutputStream(dest)) {
                byte[] buf = new byte[8192];
                int n;
                while ((n = is.read(buf)) != -1) fos.write(buf);
            }
            Log.i(TAG, "downloadToFile: saved to " + destPath);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "downloadToFile: exception", e);
            new File(destPath).delete();
            return false;
        }
    }

    // ── Playback ──

    public static void playAyah(int surah, int ayah) {
        synchronized (sPlayerLock) {
            if (sMediaPlayer != null) {
                sMediaPlayer.stop();
                sMediaPlayer.release();
                sMediaPlayer = null;
            }
        }

        new Thread(() -> {
            String localPath = getLocalAudioPath(surah, ayah);

            // Use cached file if it exists
            if (localPath != null && new File(localPath).exists()) {
                Log.i(TAG, "playAyah: playing from cache " + localPath);
                playFromPath(localPath);
                return;
            }

            // Resolve remote URL
            int reciterId = sReciters[sCurrentReciterIndex].apiId;
            String slug = sReciters[sCurrentReciterIndex].cdnSlug;
            String path = fetchAudioUrl(surah, ayah, reciterId);

            if (path == null) {
                Log.i(TAG, "playAyah: API failed, trying CDN fallback");
                path = CDN_BASE + "/" + slug + "/" + String.format("%03d%03d.mp3", surah, ayah);
            } else if (!path.startsWith("http")) {
                path = CDN_BASE + "/" + path;
            }

            // Download to cache then play
            if (localPath != null && downloadToFile(path, localPath)) {
                playFromPath(localPath);
            } else {
                Log.w(TAG, "playAyah: download failed, streaming from URL");
                playFromPath(path);
            }
        }).start();
    }

    private static void playFromPath(String path) {
        synchronized (sPlayerLock) {
            sMediaPlayer = new MediaPlayer();
            try {
                sMediaPlayer.setDataSource(path);
                sMediaPlayer.prepareAsync();
                sMediaPlayer.setOnPreparedListener(mp -> {
                    Log.i(TAG, "playFromPath: prepared, starting playback");
                    mp.start();
                });
                sMediaPlayer.setOnCompletionListener(mp -> {
                    synchronized (sPlayerLock) {
                        mp.release();
                        sMediaPlayer = null;
                    }
                });
                sMediaPlayer.setOnErrorListener((mp, what, extra) -> {
                    Log.e(TAG, "playFromPath: error what=" + what + " extra=" + extra);
                    synchronized (sPlayerLock) {
                        mp.release();
                        sMediaPlayer = null;
                    }
                    return true;
                });
            } catch (IOException e) {
                Log.e(TAG, "playFromPath: setDataSource exception", e);
            }
        }
    }

    // ── Native methods ──

    public native void nativeOnSurfaceCreated(AssetManager assetManager, float density);
    public native void nativeOnSurfaceChanged(int width, int height);
    public native void nativeOnDrawFrame();
    public native void nativeSetStatusBarHeight(float heightDp);
    public native void nativeOnTouch(float x, float y, boolean down);

    private GLSurfaceView glView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sContext = getApplicationContext();

        glView = new GLSurfaceView(this);

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
