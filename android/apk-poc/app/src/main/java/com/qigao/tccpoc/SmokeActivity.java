package com.qigao.tccpoc;

import android.app.Activity;
import android.os.Bundle;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

public final class SmokeActivity extends Activity {
    static {
        // Prepackaged Android native libraries, not generated executable code.
        System.loadLibrary("tcc");
        System.loadLibrary("poc_tcc_jni");
    }

    private native String compileObjectInSandbox(String directory);

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        String verdict;
        try {
            verdict = compileObjectInSandbox(getFilesDir().getAbsolutePath());
        } catch (Throwable ex) {
            verdict = "FAIL:JNI:" + ex.getClass().getSimpleName();
        }
        try (FileOutputStream output = openFileOutput("status.txt", MODE_PRIVATE)) {
            output.write((verdict + "\n").getBytes(StandardCharsets.UTF_8));
        } catch (Exception ex) {
            throw new IllegalStateException("Cannot write POC verdict", ex);
        }
        finish();
    }
}
