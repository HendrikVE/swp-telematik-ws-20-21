package de.vanappsteer.windowalarmconfig.adapter;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;

import java.util.HashMap;
import java.util.UUID;

import de.vanappsteer.windowalarmconfig.fragments.ConfigFragment;
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

    public PagerAdapter(FragmentManager fm, int numberOfTabs, HashMap<UUID, ConfigFragment.ConfigDescription> characteristicHashMap) {

        super(fm);
        this.mNumerOfTabs = numberOfTabs;

        mDeviceConfigFragment = DeviceConfigFragment.newInstance(
                characteristicHashMap.get(DeviceConfigFragment.BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID).getValue(),
                characteristicHashMap.get(DeviceConfigFragment.BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID).getValue()
        );

        mOtaConfigFragment = OtaConfigFragment.newInstance(
                characteristicHashMap.get(OtaConfigFragment.BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID).getValue(),
                characteristicHashMap.get(OtaConfigFragment.BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID).getValue(),
                characteristicHashMap.get(OtaConfigFragment.BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID).getValue(),
                characteristicHashMap.get(OtaConfigFragment.BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID).getValue()
        );

        mWifiConfigFragment = WifiConfigFragment.newInstance(
                characteristicHashMap.get(WifiConfigFragment.BLE_CHARACTERISTIC_CONFIG_WIFI_SSID_UUID).getValue(),
                characteristicHashMap.get(WifiConfigFragment.BLE_CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID).getValue()
        );

        mMqttConfigFragment = MqttConfigFragment.newInstance(
                characteristicHashMap.get(MqttConfigFragment.BLE_CHARACTERISTIC_CONFIG_MQTT_USER_UUID).getValue(),
                characteristicHashMap.get(MqttConfigFragment.BLE_CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID).getValue(),
                characteristicHashMap.get(MqttConfigFragment.BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID).getValue(),
                characteristicHashMap.get(MqttConfigFragment.BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID).getValue()
        );

        mSensorConfigFragment = SensorConfigFragment.newInstance(
                characteristicHashMap.get(SensorConfigFragment.BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID).getValue()
        );
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
