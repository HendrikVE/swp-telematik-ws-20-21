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

public class WifiConfigFragment extends ConfigFragment {

    public static final String KEY_BLE_CHARACTERISTIC_CONFIG_WIFI_SSID_UUID = "BLE_CHARACTERISTIC_CONFIG_WIFI_SSID_UUID";
    public static final String KEY_BLE_CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID = "BLE_CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID";

    public static final UUID BLE_CHARACTERISTIC_CONFIG_WIFI_SSID_UUID = UUID.fromString("8ca0bf1d-bb5d-4a66-9191-341fd805e288");
    public static final UUID BLE_CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID = UUID.fromString("fa41c195-ae99-422e-8f1f-0730702b3fc5");

    private TextInputEditText mEditTextWifiSsid;
    private TextInputEditText mEditTextWifiPassword;

    private String mWifiSsid = "";
    private String mWifiPassword = "";

    public WifiConfigFragment() {
        // Required empty public constructor
    }

    public static WifiConfigFragment newInstance(String ssid, String password) {

        Bundle bundle = new Bundle();
        bundle.putString(KEY_BLE_CHARACTERISTIC_CONFIG_WIFI_SSID_UUID, ssid);
        bundle.putString(KEY_BLE_CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID, password);

        WifiConfigFragment fragment = new WifiConfigFragment();
        fragment.setArguments(bundle);

        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        return inflater.inflate(R.layout.fragment_wifi_config, container, false);
    }

    @Override
    public void onActivityCreated (Bundle savedInstanceState) {

        super.onActivityCreated(savedInstanceState);

        initViews();

        Bundle bundle = getArguments();
        if (bundle != null) {
            mWifiSsid = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_WIFI_SSID_UUID);
            mWifiPassword = getArguments().getString(KEY_BLE_CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID);
        }

        mEditTextWifiSsid.setText(mWifiSsid);
        mEditTextWifiPassword.setText(mWifiPassword);
    }

    @Override
    public void onDestroyView() {

        mEditTextWifiSsid = null;
        mEditTextWifiPassword = null;

        super.onDestroyView();
    }

    @Override
    public Map<UUID, String> getInputData() {

        Map<UUID, String> map = new HashMap<>();
        map.put(BLE_CHARACTERISTIC_CONFIG_WIFI_SSID_UUID, mWifiSsid);
        map.put(BLE_CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID, mWifiPassword);

        return map;
    }

    private void  initViews() {
        mEditTextWifiSsid = getView().findViewById(R.id.editTextWifiSsid);
        mEditTextWifiSsid.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mWifiSsid = editable.toString();
            }
        });

        mEditTextWifiPassword = getView().findViewById(R.id.editTextWifiPassword);
        mEditTextWifiPassword.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mWifiPassword = editable.toString();
            }
        });
    }
}
