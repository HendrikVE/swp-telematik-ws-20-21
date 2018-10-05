package de.vanappsteer.windowalarmconfig.activities;

import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.Html;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import de.vanappsteer.windowalarmconfig.R;
import de.vanappsteer.windowalarmconfig.util.LoggingUtil;

public class AboutApp extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about_app);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        String versionName = "";
        int versionCode = 0;
        try {
            versionName = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
            versionCode = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
        }
        catch (PackageManager.NameNotFoundException e) {
            LoggingUtil.error(e.getMessage());
        }

        TextView textViewVersion = findViewById(R.id.textViewVersion);
        textViewVersion.setText(versionName + " (" + versionCode + ")");

        LinearLayout linearLayoutLicense = findViewById(R.id.linearLayoutLicense);
        linearLayoutLicense.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                AlertDialog.Builder builder = new AlertDialog.Builder(AboutApp.this);
                builder.setPositiveButton(R.string.button_ok, null);
                builder.setTitle(R.string.license_title);
                builder.setMessage(Html.fromHtml(getString(R.string.license_text)));

                AlertDialog dialog = builder.create();
                dialog.show();
            }
        });
    }

}