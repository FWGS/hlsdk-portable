package su.xash.hlsdk;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;

import android.widget.Button;
import android.widget.EditText;

public class LauncherActivity extends Activity {
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_launcher);

		EditText launchParameters = (EditText) findViewById(R.id.launchParameters);
		Button launchButton = (Button) findViewById(R.id.launchButton);

		launchParameters.setText(BuildConfig.DEFAULT_PARAMS);

		launchButton.setText(getString(R.string.launch, getString(R.string.app_name)));
		launchButton.setOnClickListener((view) -> {
			startActivity(new Intent().setComponent(new ComponentName(BuildConfig.ENGINE_PKG, "XashActivity"))
					.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
					.putExtra("gamedir", BuildConfig.GAMEDIR)
					.putExtra("argv", launchParameters.getText())
					.putExtra("gamelibdir", getApplicationInfo().nativeLibraryDir));
		});
	}
}