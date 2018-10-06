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

public class OtaConfigFragment extends ConfigFragment {

    public static final String KEY_BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID = "BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID";
    public static final String KEY_BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID = "BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID";
    public static final String KEY_BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID = "BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID";
    public static final String KEY_BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID = "BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID";

    public static final UUID BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID = UUID.fromString("2f44b103-444c-48f5-bf60-91b81dfa0a25");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID = UUID.fromString("4b95d245-db08-4c56-98f9-738faa8cfbb6");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID = UUID.fromString("1c93dce2-3796-4027-9f55-6d251c41dd34");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID = UUID.fromString("0e837309-5336-45a3-9b69-d0f7134f30ff");

    private TextInputEditText mEditTextOtaServerAddress;
    private TextInputEditText mEditTextOtaFilename;
    private TextInputEditText mEditTextOtaUsername;
    private TextInputEditText mEditTextOtaPassword;

    private String mOtaServerAddress = "";
    private String mOtaFilename = "";
    private String mOtaUsername = "";
    private String mOtaPassword = "";

    public OtaConfigFragment() {
        // Required empty public constructor
    }

    public static OtaConfigFragment newInstance(String host, String filename, String username, String password) {

        Bundle bundle = new Bundle();
        bundle.putString(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID, host);
        bundle.putString(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID, filename);
        bundle.putString(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID, username);
        bundle.putString(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID, password);

        OtaConfigFragment fragment = new OtaConfigFragment();
        fragment.setArguments(bundle);

        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        return inflater.inflate(R.layout.fragment_ota_config, container, false);
    }

    @Override
    public void onActivityCreated (Bundle savedInstanceState) {

        super.onActivityCreated(savedInstanceState);

        initViews();

        Bundle bundle = getArguments();
        if (bundle != null) {
            mOtaServerAddress = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID);
            mOtaFilename = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID);
            mOtaUsername = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID);
            mOtaPassword = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID);
        }

        mEditTextOtaServerAddress.setText(mOtaServerAddress);
        mEditTextOtaFilename.setText(mOtaFilename);
        mEditTextOtaUsername.setText(mOtaUsername);
        mEditTextOtaPassword.setText(mOtaPassword);
    }

    @Override
    public void onDestroyView() {

        mEditTextOtaServerAddress = null;
        mEditTextOtaFilename = null;
        mEditTextOtaUsername = null;
        mEditTextOtaPassword = null;

        super.onDestroyView();
    }

    @Override
    public Map<String, ConfigDescription> getInputData() {

        Map<String, ConfigDescription> map = new HashMap<>();
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID, mOtaServerAddress));
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID, mOtaFilename));
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID, mOtaUsername));
        map.put(KEY_BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID, new ConfigDescription(BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID, mOtaPassword));

        return map;
    }

    private void initViews() {
        mEditTextOtaServerAddress = getView().findViewById(R.id.editTextOtaServerAddress);
        mEditTextOtaServerAddress.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mOtaServerAddress = editable.toString();
            }
        });

        mEditTextOtaFilename = getView().findViewById(R.id.editTextOtaFilename);
        mEditTextOtaFilename.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mOtaFilename = editable.toString();
            }
        });

        mEditTextOtaUsername = getView().findViewById(R.id.editTextOtaUsername);
        mEditTextOtaUsername.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mOtaUsername = editable.toString();
            }
        });

        mEditTextOtaPassword = getView().findViewById(R.id.editTextOtaPassword);
        mEditTextOtaPassword.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mOtaPassword = editable.toString();
            }
        });
    }
}
