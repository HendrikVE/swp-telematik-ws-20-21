package de.vanappsteer.windowalarmconfig.models;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

public class DeviceConfigModel extends ConfigModel {

    public static final UUID BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID = UUID.fromString("d3491796-683b-4b9c-aafb-f51a35459d43");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID = UUID.fromString("4745e11f-b403-4cfb-83bb-710d46897875");

    private String mDeviceRoom;
    private String mDeviceID;

    public DeviceConfigModel(String room, String id) {
        mDeviceRoom = room;
        mDeviceID = id;
    }

    @Override
    public Map<UUID, String> getInputData() {

        Map<UUID, String> map = new HashMap<>();
        map.put(BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID, mDeviceRoom);
        map.put(BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID, mDeviceID);

        return map;
    }

    public static boolean includesFullDataSet(Map<UUID, String> map) {

        boolean valid;
        valid = map.containsKey(BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID);
        valid &= map.containsKey(BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID);

        return valid;
    }


    /* BEGIN GETTER */
    public String getDeviceRoom() {
        return mDeviceRoom;
    }

    public String getDeviceId() {
        return mDeviceID;
    }
    /* END GETTER */


    /* BEGIN SETTER */
    public void setDeviceRoom(String room) {
        mDeviceRoom = room;
    }

    public void setDeviceId(String id) {
        mDeviceID = id;
    }
    /* END SETTER */
}
