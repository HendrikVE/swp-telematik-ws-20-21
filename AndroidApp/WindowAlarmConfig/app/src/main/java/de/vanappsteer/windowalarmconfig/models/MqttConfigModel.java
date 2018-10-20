package de.vanappsteer.windowalarmconfig.models;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

public class MqttConfigModel extends ConfigModel {

    public static final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_USER_UUID = UUID.fromString("69150609-18f8-4523-a41f-6d9a01d2e08d");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID = UUID.fromString("8bebec77-ea21-4c14-9d64-dbec1fd5267c");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID = UUID.fromString("e3b150fb-90a2-4cd3-80c5-b1189e110ef1");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID = UUID.fromString("4eeff953-0f5e-43ee-b1be-1783a8190b0d");

    private String mMqttUsername;
    private String mMqttPassword;
    private String mMqttBrokerAddress;
    private String mMqttBrokerPort;

    public MqttConfigModel(String username, String password, String brokerAddress, String brokerPort) {
        mMqttUsername = username;
        mMqttPassword = password;
        mMqttBrokerAddress = brokerAddress;
        mMqttBrokerPort = brokerPort;
    }

    @Override
    public Map<UUID, String> getDataMap() {

        Map<java.util.UUID, java.lang.String> map = new HashMap<>();
        map.put(BLE_CHARACTERISTIC_CONFIG_MQTT_USER_UUID, mMqttUsername);
        map.put(BLE_CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID, mMqttPassword);
        map.put(BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID, mMqttBrokerAddress);
        map.put(BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID, mMqttBrokerPort);

        return map;
    }


    /* BEGIN GETTER */
    public String getMqttUsername() {
        return mMqttUsername;
    }

    public String getMqttPassword() {
        return mMqttPassword;
    }

    public String getMqttBrokerAddress() {
        return mMqttBrokerAddress;
    }

    public String getMqttBrokerPort() {
        return mMqttBrokerPort;
    }
    /* END GETTER */


    /* BEGIN SETTER */
    public void setMqttUsername(String username) {
        mMqttUsername = username;
    }

    public void setMqttPassword(String password) {
        mMqttPassword = password;
    }

    public void setMqttBrokerAddress(String address) {
        mMqttBrokerAddress = address;
    }

    public void setMqttBrokerPort(String port) {
        mMqttBrokerPort = port;
    }
    /* END SETTER */
}
