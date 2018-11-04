package in.celest.xash3d;

import android.app.*;
import android.content.*;
import android.graphics.*;
import android.os.*;
import android.text.*;
import android.text.style.*;
import android.view.*;
import android.widget.*;
import android.widget.LinearLayout.*;

public class LauncherActivity extends Activity {
	static EditText cmdArgs;
	static SharedPreferences mPref;
	public static final int sdk = Integer.valueOf(Build.VERSION.SDK);
	
	public SpannableString styleButtonString(String str)
	{
		if(sdk < 21)
			str = str.toUpperCase();

		SpannableString spanString = new SpannableString(str.toUpperCase());
		
		if(sdk < 21)
			spanString.setSpan(new StyleSpan(Typeface.BOLD), 0, str.length(), 0);

		return spanString;
	}
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		this.requestWindowFeature(Window.FEATURE_NO_TITLE);

		// set theme
		if ( sdk >= 21 )
			super.setTheme( 0x01030224 );
		else super.setTheme( 0x01030005 );

        // Build layout
        LinearLayout launcher = new LinearLayout(this);
        launcher.setOrientation(LinearLayout.VERTICAL);
        launcher.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

		launcher.setBackgroundColor(0xFF252525);
		TextView launcherTitle = new TextView(this);
        LayoutParams titleparams = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
		/* тут*/titleparams.setMargins(5,15,5,1);//размеры верхнего layout
		LayoutParams buttonparams = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
		buttonparams.setMargins(10,20,10,20);//размеры строки для ввода аргументов
		launcherTitle.setLayoutParams(titleparams);
        launcherTitle.setText("Crack-Life: Campaign Mode");
        launcherTitle.setGravity(1);
        //launcherTitle.setTextColor(0xFF4db017);
        launcherTitle.setTextAppearance(this, android.R.attr.textAppearanceMedium);
		launcherTitle.setTextSize(25);
		launcherTitle.setBackgroundColor(0xFF555555);
		/*launcherTitle.setCompoundDrawablePadding(10);
		try
		{
			launcherTitle.setCompoundDrawablesWithIntrinsicBounds(getApplicationContext().getPackageManager().getApplicationIcon(getPackageName()),null,null,null);
			launcherTitle.setPadding(9,9,6,0);
		}
		catch(Exception e)
		{
			launcherTitle.setPadding(60,6,6,6);
		}*/
		launcher.addView(launcherTitle);
		LinearLayout launcherBody = new LinearLayout(this);
        launcherBody.setOrientation(LinearLayout.VERTICAL);
        launcherBody.setLayoutParams(titleparams);
		launcherBody.setBackgroundColor(0xFF454545);
		LinearLayout launcherBorder = new LinearLayout(this);
		launcherBorder.setLayoutParams(titleparams);
		launcherBorder.setBackgroundColor(0xFF555555);
		launcherBorder.setOrientation(LinearLayout.VERTICAL);

		LinearLayout launcherBorder2 = new LinearLayout(this);
		launcherBorder2.setLayoutParams(titleparams);
		launcherBorder2.setOrientation(LinearLayout.VERTICAL);
		launcherBorder2.setBackgroundColor(0xFF353535);
		launcherBorder2.addView(launcherBody);
		launcherBorder2.setPadding(10,0,10,10);
		launcherBorder.addView(launcherBorder2);
		launcherBorder.setPadding(10,0,10,20);
		launcher.addView(launcherBorder);

        TextView titleView = new TextView(this);
        titleView.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
        titleView.setText("Command-line arguments");
        titleView.setTextAppearance(this, android.R.attr.textAppearanceLarge);

		cmdArgs = new EditText(this);
        cmdArgs.setLayoutParams(buttonparams);
		cmdArgs.setSingleLine(true);

		if(sdk < 21)
		{
			cmdArgs.setBackgroundColor(0xFF353535);
			cmdArgs.setTextColor(0xFF333333);
			cmdArgs.setPadding(5,5,5,5);
		}

		RelativeLayout panel = new RelativeLayout(this);
		panel.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
		
		Button startButton = new Button(this);

		// Set launch button title here
		startButton.setText(styleButtonString("Launch " + "Campaign Mode" + "!"));
		RelativeLayout.LayoutParams buttonParams = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
		buttonParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
		startButton.setLayoutParams(buttonParams);
		if(sdk < 21)
		{
			startButton.getBackground().setAlpha(96);
			startButton.getBackground().invalidateSelf();
			startButton.setTextColor(0xFFFFFFFF);
			startButton.setTextAppearance(this, android.R.attr.textAppearanceLarge);
			startButton.setTextSize(20);
		}
		startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startXash(v);
            }
        });
		launcherBody.addView(titleView);
		launcherBody.addView(cmdArgs);

		// Add other options here

		panel.addView(startButton);
		launcher.addView(panel);
        setContentView(launcher);
		mPref = getSharedPreferences("mod", 0);
		cmdArgs.setText(mPref.getString("argv","-dev 3 -log"));

		//ExtractAssets.extractPAK(this, false);
	}

	private Intent prepareIntent(Intent i)
	{
		String argv = cmdArgs.getText().toString();
		i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

		SharedPreferences.Editor editor = mPref.edit();

		editor.putString("argv", argv);
		editor.commit();

		// Command-line arguments
		// if not passed, uses arguments from xash3d package
		// Uncoment this if you are using client from other package
		/*
		String libserver = getFilesDir().getAbsolutePath().replace("/files","/lib/libserver_hardfp.so");
		if( !(new File(libserver).exists()) )
			libserver = getFilesDir().getAbsolutePath().replace("/files","/lib/libserver.so");
		argv = "-dll "+ libserver + " " + argv;
		*/
		if(argv.length() != 0)
			i.putExtra("argv", argv);

		// default gamedir
		// may be overriden by -game command-line option
		// Uncomment to set gamedir here
		i.putExtra("gamedir", "clcampaign");

		// default library package
		// if you are using client from other package (not from half-life),
		// replace following line by:
		/*i.putExtra("gamelibdir", "/data/data/<clientpkgname>/lib");*/
		i.putExtra("gamelibdir", getFilesDir().getAbsolutePath().replace("/files","/lib"));
		//i.putExtra("pakfile", getFilesDir().getAbsolutePath() + "/extras.pak");

		// you may pass extra enviroment variables to game
		// it is availiable from game code with getenv() function
		/*
		String envp[] = 
		{
			"VAR1", "value1",
			"VAR2", "value2"
		};
		i.putExtra("env", envp);
		*/

		return i;
	}
	
    public void startXash(View view)
    {
		try
		{
			Intent intent = new Intent();
			intent.setAction("in.celest.xash3d.START");
			intent = prepareIntent(intent);
			startActivity(intent);
			return;
		}
		catch(Exception e){}
		// some samsung devices have 
		// completely broken intent resolver
		// so try specify exact component here
		try
		{
			Intent intent = new Intent();
			intent.setComponent(new ComponentName("in.celest.xash3d.hl.test", "in.celest.xash3d.XashActivity")); 
			intent = prepareIntent(intent);
			startActivity(intent);
			return;
		}
		catch(Exception e){}
		try
		{
			Intent intent = new Intent();
			intent.setComponent(new ComponentName("in.celest.xash3d.hl", "in.celest.xash3d.XashActivity")); 
			intent = prepareIntent(intent);
			startActivity(intent);
			return;
		}
		catch(Exception e){}
		new AlertDialog.Builder(this)
		.setTitle("Error")
		.setMessage("Failed to start Xash3D FWGS engine\nIs it installed?")
		.setCancelable(false)
		.setPositiveButton("Ok", new DialogInterface.OnClickListener()
		{
			public void onClick(DialogInterface i, int w){
				LauncherActivity.this.finish();
			}
		}).show();
	}
}
