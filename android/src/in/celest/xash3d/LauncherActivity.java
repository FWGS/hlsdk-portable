package in.celest.xash3d;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.content.Intent;
import android.widget.EditText;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.Button;
import android.widget.TextView;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.content.SharedPreferences;
import android.content.Context;
import android.util.Log;

public class LauncherActivity extends Activity {
	private static final int PAK_VERSION = 1;
	static Boolean isExtracting = false;
	static EditText cmdArgs;
	static SharedPreferences mPref;
	private static final String TAG = "MOD_LAUNCHER";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Build layout
        LinearLayout launcher = new LinearLayout(this);
        launcher.setOrientation(LinearLayout.VERTICAL);
        launcher.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
        TextView titleView = new TextView(this);
        titleView.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
        titleView.setText("Command-line arguments");
        titleView.setTextAppearance(this, android.R.attr.textAppearanceLarge);
        cmdArgs = new EditText(this);
        cmdArgs.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
		Button startButton = new Button(this);
		// Set launch button title here
		startButton.setText("Launch with gravgun!");
		LayoutParams buttonParams = new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		buttonParams.gravity = 5;
		startButton.setLayoutParams(buttonParams);
		startButton.setOnClickListener( new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startXash(v);
            }
        });
		launcher.addView(titleView);
		launcher.addView(cmdArgs);
		// Add other options here
		launcher.addView(startButton);
        setContentView(launcher);
		mPref = getSharedPreferences("mod", 0);
		InstallReceiver.extractPAK(this, false);
		cmdArgs.setText(mPref.getString("argv","-dev 3 -log"));
	}

    public void startXash(View view)
    {
		Intent intent = new Intent();
		intent.setAction("in.celest.xash3d.START");
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

		SharedPreferences.Editor editor = mPref.edit();
		editor.putString("argv", cmdArgs.getText().toString());
		editor.commit();
		editor.apply();
		if(cmdArgs.length() != 0) intent.putExtra("argv", cmdArgs.getText().toString());
		// Uncomment to set gamedir here
		// intent.putExtra("gamedir", "mod" );
		intent.putExtra("gamelibdir", getFilesDir().getAbsolutePath().replace("/files","/lib"));
		intent.putExtra("pakfile", getFilesDir().getAbsolutePath() + "/extras.pak" );
		startActivity(intent);
    }
}
