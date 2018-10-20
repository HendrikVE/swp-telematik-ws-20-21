package de.vanappsteer.windowalarmconfig.models;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

public class WifiConfigModel extends ConfigModel {

    public static final UUID BLE_CHARACTERISTIC_CONFIG_WIFI_SSID_UUID = UUID.fromString("8ca0bf1d-bb5d-4a66-9191-341fd805e288");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID = UUID.fromString("fa41c195-ae99-422e-8f1f-0730702b3fc5");

    private String mWifiSsid;
    private String mWifiPassword;

    public WifiConfigModel(String ssid, String password) {
        mWifiSsid = ssid;
        mWifiPassword = password;
    }

    @Override
    public Map<UUID, String> getInputData() {

        Map<UUID, String> map = new HashMap<>();
        map.put(BLE_CHARACTERISTIC_CONFIG_WIFI_SSID_UUID, mWifiSsid);
        map.put(BLE_CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID, mWifiPassword);

        return map;
    }


    /* BEGIN GETTER */
    public String getWifiSsid() {
        return mWifiSsid;
    }

    public String getWifiPassword() {
        return mWifiPassword;
    }
    /* END GETTER */


    /* BEGIN SETTER */
    public void setWifiSsid(String ssid) {
        mWifiSsid = ssid;
    }

    public void setWifiPassword(String password) {
        mWifiPassword = password;
    }
    /* END SETTER */
}
