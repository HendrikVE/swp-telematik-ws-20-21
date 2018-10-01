package de.vanappsteer.windowalarmconfig.fragments;


import android.os.Bundle;
import android.support.design.widget.TextInputEditText;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import de.vanappsteer.windowalarmconfig.R;
import de.vanappsteer.windowalarmconfig.util.LoggingUtil;

public class MqttConfigFragment extends ConfigFragment {

    public final String KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_USER_UUID = "BLE_CHARACTERISTIC_CONFIG_MQTT_USER_UUID";
    public final String KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID = "BLE_CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID";
    public final String KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID = "BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID";
    public final String KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID = "BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID";

    private final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_USER_UUID = UUID.fromString("69150609-18f8-4523-a41f-6d9a01d2e08d");
    private final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID = UUID.fromString("8bebec77-ea21-4c14-9d64-dbec1fd5267c");
    private final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID = UUID.fromString("e3b150fb-90a2-4cd3-80c5-b1189e110ef1");
    private final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID = UUID.fromString("4eeff953-0f5e-43ee-b1be-1783a8190b0d");

    private TextInputEditText mEditTextMqttUsername;
    private TextInputEditText mEditTextMqttPassword;
    private TextInputEditText mEditTextMqttBrokerAddress;
    private TextInputEditText mEditTextMqttBrokerPort;

    private String mMqttUsername = "";
    private String mMqttPassword = "";
    private String mMqttBrokerAddress = "";
    private String mMqttBrokerPort = "";

    public MqttConfigFragment() {
        // Required empty public constructor
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        return inflater.inflate(R.layout.fragment_mqtt_config, container, false);
    }

    @Override
    public void onActivityCreated (Bundle savedInstanceState) {

        super.onActivityCreated(savedInstanceState);

        initViews();

        Bundle bundle = getArguments();
        if (bundle != null) {
            mMqttUsername = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_USER_UUID);
            mMqttPassword = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID);
            mMqttBrokerAddress = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID);
            mMqttBrokerPort = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID);

            mEditTextMqttUsername.setText(mMqttUsername);
            mEditTextMqttPassword.setText(mMqttPassword);
            mEditTextMqttBrokerAddress.setText(mMqttBrokerAddress);
            mEditTextMqttBrokerPort.setText(mMqttBrokerPort);
        }
    }

    @Override
    public void onDestroyView() {

        mEditTextMqttUsername = null;
        mEditTextMqttPassword = null;
        mEditTextMqttBrokerAddress = null;
        mEditTextMqttBrokerPort = null;

        super.onDestroyView();
    }

    @Override
    public Map<String, ConfigDescription> getInputData() {

        Map<String, ConfigDescription> map = new HashMap<>();
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_USER_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_MQTT_USER_UUID, mMqttUsername));
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID, mMqttPassword));
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID, mMqttBrokerAddress));
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID, mMqttBrokerPort));

        return map;
    }

    private void initViews() {

        mEditTextMqttUsername = getView().findViewById(R.id.editTextMqttUsername);
        mEditTextMqttUsername.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                mMqttUsername = editable.toString();
            }
        });

        mEditTextMqttPassword = getView().findViewById(R.id.editTextMqttPassword);
        mEditTextMqttPassword.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                mMqttPassword = editable.toString();
            }
        });

        mEditTextMqttBrokerAddress = getView().findViewById(R.id.editTextMqttBrokerAddress);
        mEditTextMqttBrokerAddress.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                mMqttBrokerAddress = editable.toString();
            }
        });

        mEditTextMqttBrokerPort = getView().findViewById(R.id.editTextMqttBrokerPort);
        mEditTextMqttBrokerPort.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                mMqttBrokerPort = editable.toString();
            }
        });
    }
}
