package de.vanappsteer.windowalarmconfig.models;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

public class OtaConfigModel extends ConfigModel {

    public static final UUID BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID = UUID.fromString("2f44b103-444c-48f5-bf60-91b81dfa0a25");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID = UUID.fromString("4b95d245-db08-4c56-98f9-738faa8cfbb6");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID = UUID.fromString("1c93dce2-3796-4027-9f55-6d251c41dd34");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID = UUID.fromString("0e837309-5336-45a3-9b69-d0f7134f30ff");

    private String mOtaServerAddress;
    private String mOtaFilename;
    private String mOtaUsername;
    private String mOtaPassword;

    public OtaConfigModel(String serverAddress, String filename, String username, String password) {
        mOtaServerAddress = serverAddress;
        mOtaFilename = filename;
        mOtaUsername = username;
        mOtaPassword = password;
    }

    @Override
    public Map<UUID, String> getInputData() {

        Map<UUID, String> map = new HashMap<>();
        map.put(BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID, mOtaServerAddress);
        map.put(BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID, mOtaFilename);
        map.put(BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID, mOtaUsername);
        map.put(BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID, mOtaPassword);

        return map;
    }


    /* BEGIN GETTER */
    public String getOtaServerAddress() {
        return mOtaServerAddress;
    }

    public String getOtaFilename() {
        return mOtaFilename;
    }

    public String getOtaUsername() {
        return mOtaUsername;
    }

    public String getOtaPassword() {
        return mOtaPassword;
    }
    /* END GETTER */


    /* BEGIN SETTER */
    public void setOtaServerAddress(String address) {
        mOtaServerAddress = address;
    }

    public void setOtaFilename(String filename) {
        mOtaFilename = filename;
    }

    public void setOtaUsername(String username) {
        mOtaUsername = username;
    }

    public void setOtaPassword(String password) {
        mOtaPassword = password;
    }
    /* END SETTER */
}
