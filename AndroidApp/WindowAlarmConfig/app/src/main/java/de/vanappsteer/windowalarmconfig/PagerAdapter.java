package de.vanappsteer.windowalarmconfig;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;

import de.vanappsteer.windowalarmconfig.fragments.DeviceConfigFragment;
import de.vanappsteer.windowalarmconfig.fragments.MqttConfigFragment;
import de.vanappsteer.windowalarmconfig.fragments.OtaConfigFragment;
import de.vanappsteer.windowalarmconfig.fragments.SensorConfigFragment;
import de.vanappsteer.windowalarmconfig.fragments.WifiConfigFragment;
import de.vanappsteer.windowalarmconfig.util.LoggingUtil;

public class PagerAdapter extends FragmentStatePagerAdapter {

    int mNumerOfTabs;

    public PagerAdapter(FragmentManager fm, int numberOfTabs) {
        super(fm);
        this.mNumerOfTabs = numberOfTabs;
    }

    @Override
    public Fragment getItem(int position) {

        Fragment fragment;

        switch (position) {

            case 0:
                fragment = new DeviceConfigFragment();
                break;

            case 1:
                fragment = new OtaConfigFragment();
                break;

            case 2:
                fragment = new WifiConfigFragment();
                break;

            case 3:
                fragment = new MqttConfigFragment();
                break;

            case 4:
                fragment = new SensorConfigFragment();
                break;

            default:
                LoggingUtil.error("no fragment defined for position: " + position);
                return null;
        }

        return fragment;
    }

    @Override
    public int getCount() {
        return mNumerOfTabs;
    }
}
