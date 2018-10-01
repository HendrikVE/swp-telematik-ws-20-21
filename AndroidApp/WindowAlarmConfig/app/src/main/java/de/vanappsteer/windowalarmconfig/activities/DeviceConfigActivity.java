package de.vanappsteer.windowalarmconfig.activities;

import android.os.Bundle;
import android.support.design.widget.TabLayout;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.widget.Button;

import java.util.Map;

import de.vanappsteer.windowalarmconfig.adapter.PagerAdapter;
import de.vanappsteer.windowalarmconfig.R;
import de.vanappsteer.windowalarmconfig.fragments.ConfigFragment;
import de.vanappsteer.windowalarmconfig.util.LoggingUtil;

public class DeviceConfigActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_config);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        TabLayout tabLayout = findViewById(R.id.tabLayout);
        tabLayout.setTabGravity(TabLayout.GRAVITY_FILL);

        final ViewPager viewPager = findViewById(R.id.pager);
        final PagerAdapter adapter = new PagerAdapter(getSupportFragmentManager(), tabLayout.getTabCount());
        viewPager.setAdapter(adapter);
        viewPager.addOnPageChangeListener(new TabLayout.TabLayoutOnPageChangeListener(tabLayout));
        tabLayout.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
            @Override
            public void onTabSelected(TabLayout.Tab tab) {
                viewPager.setCurrentItem(tab.getPosition());
            }

            @Override
            public void onTabUnselected(TabLayout.Tab tab) {

            }

            @Override
            public void onTabReselected(TabLayout.Tab tab) {

            }
        });

        Button buttonSave = findViewById(R.id.buttonSave);
        buttonSave.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                for (int i = 0; i < adapter.getCount(); i++) {
                    ConfigFragment configFragment = (ConfigFragment) adapter.getItem(i);

                    Map<String, ConfigFragment.ConfigDescription> map = configFragment.getInputData();
                    for (Map.Entry<String, ConfigFragment.ConfigDescription> entry : map.entrySet()) {
                        LoggingUtil.debug("key: " + entry.getKey());
                        LoggingUtil.debug("UUID: " + entry.getValue().getUuid().toString());
                        LoggingUtil.debug("value: " + entry.getValue().getValue());
                        LoggingUtil.debug("");
                    }
                }

                DeviceConfigActivity.this.finish();
            }
        });
        Button buttonCancel = findViewById(R.id.buttonCancel);
        buttonCancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                DeviceConfigActivity.this.finish();
            }
        });
    }

}
