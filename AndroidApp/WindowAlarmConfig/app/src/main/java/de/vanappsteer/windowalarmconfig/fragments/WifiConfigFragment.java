package de.vanappsteer.windowalarmconfig.fragments;

import android.os.Bundle;
import android.support.design.widget.TextInputEditText;
import android.support.v4.app.Fragment;
import android.text.Editable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import de.vanappsteer.windowalarmconfig.R;
import de.vanappsteer.windowalarmconfig.controller.WifiConfigController;
import de.vanappsteer.windowalarmconfig.models.WifiConfigModel;
import de.vanappsteer.windowalarmconfig.util.TextChangeWatcher;

public class WifiConfigFragment extends Fragment implements WifiConfigController.View {

    private TextInputEditText mEditTextWifiSsid;
    private TextInputEditText mEditTextWifiPassword;

    private WifiConfigController mController;

    public WifiConfigFragment() {
        // Required empty public constructor
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        return inflater.inflate(R.layout.fragment_wifi_config, container, false);
    }

    @Override
    public void onActivityCreated (Bundle savedInstanceState) {

        super.onActivityCreated(savedInstanceState);

        initViews();
    }

    @Override
    public void onDestroyView() {

        mEditTextWifiSsid = null;
        mEditTextWifiPassword = null;

        super.onDestroyView();
    }

    private void initViews() {
        mEditTextWifiSsid = getView().findViewById(R.id.editTextWifiSsid);
        updateWifiSsid(mController.getSsid());
        mEditTextWifiSsid.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setWifiSsid(editable.toString());
            }
        });

        mEditTextWifiPassword = getView().findViewById(R.id.editTextWifiPassword);
        updateWifiPassword(mController.getPassword());
        mEditTextWifiPassword.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setWifiPassword(editable.toString());
            }
        });
    }

    @Override
    public void updateWifiSsid(String ssid) {
        mEditTextWifiSsid.setText(ssid);
    }

    @Override
    public void updateWifiPassword(String password) {
        mEditTextWifiPassword.setText(password);
    }

    @Override
    public void setModel(WifiConfigModel model) {
        mController = new WifiConfigController(model, this);
    }

    @Override
    public WifiConfigModel getModel() {
        return mController.getModel();
    }
}
