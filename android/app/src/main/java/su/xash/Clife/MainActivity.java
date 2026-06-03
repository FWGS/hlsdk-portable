package su.xash.clife;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public class MainActivity extends AppCompatActivity {
    private SharedPreferences prefs;
    private static final String PREF_NAME = "AxionPrefs";
    private static final String KEY_ARGV = "last_argv";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        prefs = getSharedPreferences(PREF_NAME, MODE_PRIVATE);

        androidx.appcompat.widget.Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        EditText argvInput = findViewById(R.id.argvInput);
        Button runButton = findViewById(R.id.runButton);
        Button infoButton = findViewById(R.id.infoButton);

        argvInput.setText(prefs.getString(KEY_ARGV, "-console -log"));

        argvInput.setSingleLine(true);
        argvInput.setImeOptions(EditorInfo.IME_ACTION_DONE);
        argvInput.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                saveArgs(argvInput.getText().toString());
                argvInput.clearFocus();
                InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                if (imm != null) {
                    imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
                }
                return true;
            }
            return false;
        });

        runButton.setOnClickListener(v -> {
            String userArgs = argvInput.getText().toString();
            saveArgs(userArgs);
            String fullArgs = userArgs + " -dll @hl";
            startGame(fullArgs.trim());
        });
        
        infoButton.setOnClickListener(v -> {
            new MaterialAlertDialogBuilder(this)
                    .setTitle(R.string.about_title)
                    .setMessage(R.string.about_text)
                    .setPositiveButton(android.R.string.ok, null)
                    .setNeutralButton(R.string.axion_link, (dialog, which) -> {
                        startActivity(new Intent(Intent.ACTION_VIEW, 
                                Uri.parse("https://github.com/Elinsrc/Axion")));
                    })
                    .show();
        });
    }

    private void saveArgs(String args) {
        prefs.edit().putString(KEY_ARGV, args).apply();
    }

    private void startGame(String argv) {
        String pkg = "su.xash.engine.test";

        try {
            getPackageManager().getPackageInfo(pkg, 0);
        } catch (PackageManager.NameNotFoundException e) {
            try {
                pkg = "su.xash.engine";
                getPackageManager().getPackageInfo(pkg, 0);
            } catch (PackageManager.NameNotFoundException ex) {
                new MaterialAlertDialogBuilder(this)
                        .setTitle(R.string.engine_not_found_title)
                        .setMessage(R.string.engine_not_found_msg)
                        .setPositiveButton(android.R.string.yes, (dialog, which) -> {
                            startActivity(new Intent(Intent.ACTION_VIEW,
                                    Uri.parse("https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous")));
                        })
                        .setNegativeButton(android.R.string.no, null)
                        .show();
                return;
            }
        }

        startActivity(new Intent().setComponent(new ComponentName(pkg, "su.xash.engine.XashActivity"))
                .putExtra("gamedir", "cracklife")
                .putExtra("gamelibdir", getApplicationInfo().nativeLibraryDir)
                .putExtra("argv", argv)
                .putExtra("package", getPackageName()));
    }
}
