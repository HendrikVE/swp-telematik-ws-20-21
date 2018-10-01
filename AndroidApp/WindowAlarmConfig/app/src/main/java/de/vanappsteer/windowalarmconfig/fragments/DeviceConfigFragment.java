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

public class DeviceConfigFragment extends ConfigFragment {

    public final String KEY_BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID = "BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID";
    public final String KEY_BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID = "BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID";

    private final UUID BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID = UUID.fromString("d3491796-683b-4b9c-aafb-f51a35459d43");
    private final UUID BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID = UUID.fromString("4745e11f-b403-4cfb-83bb-710d46897875");

    private TextInputEditText mEditTextDeviceRoom;
    private TextInputEditText mEditTextDeviceID;

    private String mDeviceRoom = "";
    private String mDeviceID = "";

    public DeviceConfigFragment() {
        // Required empty public constructor
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        return inflater.inflate(R.layout.fragment_device_config, container, false);
    }

    @Override
    public void onActivityCreated (Bundle savedInstanceState) {

        super.onActivityCreated(savedInstanceState);

        initViews();

        Bundle bundle = getArguments();
        if (bundle != null) {
            mDeviceRoom = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID);
            mDeviceID = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID);

            mEditTextDeviceRoom.setText(mDeviceRoom);
            mEditTextDeviceID.setText(mDeviceID);
        }
    }

    @Override
    public void onDestroyView() {

        mEditTextDeviceRoom = null;
        mEditTextDeviceID = null;

        super.onDestroyView();
    }

    @Override
    public Map<String, ConfigDescription> getInputData() {

        Map<String, ConfigDescription> map = new HashMap<>();
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID, mDeviceRoom));
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID, mDeviceID));

        return map;
    }

    private void initViews() {

        mEditTextDeviceRoom = getView().findViewById(R.id.editTextDeviceRoom);
        mEditTextDeviceRoom.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                mDeviceRoom = editable.toString();
            }
        });

        mEditTextDeviceID = getView().findViewById(R.id.editTextDeviceID);
        mEditTextDeviceID.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                mDeviceID = editable.toString();
            }
        });
    }
}
