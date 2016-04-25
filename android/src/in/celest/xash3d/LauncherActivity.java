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
import java.lang.reflect.Method;
import java.io.FileOutputStream;
import java.io.File;
import java.io.InputStream;
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
		extractPAK(this, false);
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
       private static int chmod(String path, int mode) {
                int ret = -1;
                try
                {
                ret = Runtime.getRuntime().exec("chmod " + Integer.toOctalString(mode) + " " + path).waitFor();
                        Log.d(TAG, "chmod " + Integer.toOctalString(mode) + " " + path + ": " + ret );
                }
                catch(Exception e)
                {
                        ret = -1;
                        Log.d(TAG, "chmod: Runtime not worked: " + e.toString() );
                }
                try
                {
                Class fileUtils = Class.forName("android.os.FileUtils");
                Method setPermissions = fileUtils.getMethod("setPermissions",
                                String.class, int.class, int.class, int.class);
                ret = (Integer) setPermissions.invoke(null, path,
                                mode, -1, -1);
                }
                catch(Exception e)
                {
                        ret = -1;
                        Log.d(TAG, "chmod: FileUtils not worked: " + e.toString() );
                }
                return ret;
        }

        private static void extractFile(Context context, String path) {
                        try
                        {
                                InputStream is = null;
                                FileOutputStream os = null;
                                is = context.getAssets().open(path);
                                File out = new File(context.getFilesDir().getPath()+'/'+path);
                                out.getParentFile().mkdirs();
                                chmod( out.getParent(), 0777 );
                                os = new FileOutputStream(out);
                                byte[] buffer = new byte[1024];
                                int length;
                                while ((length = is.read(buffer)) > 0) {
                                        os.write(buffer, 0, length);
                                }
                                os.close();
                                is.close();
                                chmod( context.getFilesDir().getPath()+'/'+path, 0777 );
                        } catch( Exception e )
                {
                        Log.e( TAG, "Failed to extract file:" + e.toString() );
                        e.printStackTrace();
                }

        }

        public static void extractPAK(Context context, Boolean force) {
                if(isExtracting)
                        return;
                isExtracting = true;
                try {
                if( mPref == null )
                        mPref = context.getSharedPreferences("mod", 0);
                if( mPref.getInt( "pakversion", 0 ) == PAK_VERSION && !force )
                        return;
                        extractFile(context, "extras.pak");

                        SharedPreferences.Editor editor = mPref.edit();
                        editor.putInt( "pakversion", PAK_VERSION );
                        editor.commit();
                        editor.apply();
                } catch( Exception e )
                {
                        Log.e( TAG, "Failed to extract PAK:" + e.toString() );
                }
                isExtracting = false;
        }

}
