package de.vanappsteer.windowalarmconfig.fragments;


import android.os.Bundle;
import android.support.design.widget.TextInputEditText;
import android.text.Editable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import de.vanappsteer.windowalarmconfig.R;
import de.vanappsteer.windowalarmconfig.util.TextChangeWatcher;

public class SensorConfigFragment extends ConfigFragment {

    public final String KEY_BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID = "BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID";

    private final UUID BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID = UUID.fromString("68011c92-854a-4f2c-a94c-5ee37dc607c3");

    private TextInputEditText mEditTextSensorPollInterval;

    private String mSensorPollInterval = "";

    public SensorConfigFragment() {
        // Required empty public constructor
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        return inflater.inflate(R.layout.fragment_sensor_config, container, false);
    }

    @Override
    public void onActivityCreated (Bundle savedInstanceState) {

        super.onActivityCreated(savedInstanceState);

        initViews();

        Bundle bundle = getArguments();
        if (bundle != null) {
            mSensorPollInterval = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID);

            mEditTextSensorPollInterval.setText(mSensorPollInterval);
        }
    }

    @Override
    public void onDestroyView() {

        mEditTextSensorPollInterval = null;

        super.onDestroyView();
    }

    @Override
    public Map<String, ConfigDescription> getInputData() {

        Map<String, ConfigDescription> map = new HashMap<>();
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID, mSensorPollInterval));

        return map;
    }

    private void initViews() {
        mEditTextSensorPollInterval = getView().findViewById(R.id.editTextSensorPollInterval);
        mEditTextSensorPollInterval.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mSensorPollInterval = editable.toString();
            }
        });
    }
}
