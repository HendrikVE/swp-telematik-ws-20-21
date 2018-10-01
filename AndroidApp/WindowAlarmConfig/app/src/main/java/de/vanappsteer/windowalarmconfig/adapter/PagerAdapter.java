package de.vanappsteer.windowalarmconfig.adapter;

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

    private int mNumerOfTabs;

    private DeviceConfigFragment mDeviceConfigFragment;
    private OtaConfigFragment mOtaConfigFragment;
    private WifiConfigFragment mWifiConfigFragment;
    private MqttConfigFragment mMqttConfigFragment;
    private SensorConfigFragment mSensorConfigFragment;

    public PagerAdapter(FragmentManager fm, int numberOfTabs) {
        super(fm);
        this.mNumerOfTabs = numberOfTabs;
        mDeviceConfigFragment = new DeviceConfigFragment();
        mOtaConfigFragment = new OtaConfigFragment();
        mWifiConfigFragment = new WifiConfigFragment();
        mMqttConfigFragment = new MqttConfigFragment();
        mSensorConfigFragment = new SensorConfigFragment();
    }

    @Override
    public Fragment getItem(int position) {

        Fragment fragment;

        switch (position) {

            case 0:
                fragment = mDeviceConfigFragment;
                break;

            case 1:
                fragment = mOtaConfigFragment;
                break;

            case 2:
                fragment = mWifiConfigFragment;
                break;

            case 3:
                fragment = mMqttConfigFragment;
                break;

            case 4:
                fragment = mSensorConfigFragment;
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
