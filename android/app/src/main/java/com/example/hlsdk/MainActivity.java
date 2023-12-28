package com.example.hlsdk;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;


public class MainActivity extends AppCompatActivity {
    ActivityResultLauncher<Intent> mActivityResultLauncher = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
        if (result.getResultCode() != Activity.RESULT_OK) {
            Log.e(TAG, result.toString());
        }
    });

    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        String pkg = "su.xash.engine.test";

        try {
            getPackageManager().getPackageInfo(pkg, 0);
        } catch (PackageManager.NameNotFoundException e) {
            try {
                pkg = "su.xash.engine";
                getPackageManager().getPackageInfo(pkg, 0);
            } catch (PackageManager.NameNotFoundException ex) {
                startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=su.xash.engine")).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK));
                finish();
                return;
            }
        }

        mActivityResultLauncher.launch(new Intent().setComponent(new ComponentName(pkg, "su.xash.engine.XashActivity"))
                .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK)
                .putExtra("gamedir", "valve")
                .putExtra("gamelibdir", getApplicationInfo().nativeLibraryDir)
                .putExtra("package", getPackageName()));
        finish();
    }
}