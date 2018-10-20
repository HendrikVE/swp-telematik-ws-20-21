package de.vanappsteer.windowalarmconfig.models;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

public class SensorConfigModel extends ConfigModel {

    public static final UUID BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID = UUID.fromString("68011c92-854a-4f2c-a94c-5ee37dc607c3");

    private String mSensorPollInterval;

    public SensorConfigModel(String sensorPollInterval) {
        mSensorPollInterval = sensorPollInterval;
    }

    @Override
    public Map<UUID, String> getInputData() {

        Map<UUID, String> map = new HashMap<>();
        map.put(BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID, mSensorPollInterval);

        return map;
    }

    public static boolean includesFullDataSet(Map<UUID, String> map) {

        boolean valid;
        valid = map.containsKey(BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID);

        return valid;
    }


    /* BEGIN GETTER */
    public String getSensorPollInterval() {
        return mSensorPollInterval;
    }
    /* END GETTER */


    /* BEGIN SETTER */
    public void setSensorPollInterval(String pollInterval) {
        mSensorPollInterval = pollInterval;
    }
    /* END SETTER */
}
