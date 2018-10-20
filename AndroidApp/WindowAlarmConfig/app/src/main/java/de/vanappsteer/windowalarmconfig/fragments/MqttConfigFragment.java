package de.vanappsteer.windowalarmconfig.fragments;

import android.os.Bundle;
import android.support.design.widget.TextInputEditText;
import android.support.v4.app.Fragment;
import android.text.Editable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import de.vanappsteer.windowalarmconfig.R;
import de.vanappsteer.windowalarmconfig.controller.MqttConfigController;
import de.vanappsteer.windowalarmconfig.models.MqttConfigModel;
import de.vanappsteer.windowalarmconfig.util.TextChangeWatcher;

public class MqttConfigFragment extends Fragment implements MqttConfigController.View {

    private TextInputEditText mEditTextMqttUsername;
    private TextInputEditText mEditTextMqttPassword;
    private TextInputEditText mEditTextMqttBrokerAddress;
    private TextInputEditText mEditTextMqttBrokerPort;

    private MqttConfigController mController;

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
    }

    @Override
    public void onDestroyView() {

        mEditTextMqttUsername = null;
        mEditTextMqttPassword = null;
        mEditTextMqttBrokerAddress = null;
        mEditTextMqttBrokerPort = null;

        super.onDestroyView();
    }

    private void initViews() {

        mEditTextMqttUsername = getView().findViewById(R.id.editTextMqttUsername);
        updateMqttUsername(mController.getMqttUsername());
        mEditTextMqttUsername.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setMqttUsername(editable.toString());
            }
        });

        mEditTextMqttPassword = getView().findViewById(R.id.editTextMqttPassword);
        updateMqttPassword(mController.getMqttPassword());
        mEditTextMqttPassword.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setMqttPassword(editable.toString());
            }
        });

        mEditTextMqttBrokerAddress = getView().findViewById(R.id.editTextMqttBrokerAddress);
        updateMqttBrokerAddress(mController.getMqttBrokerAddress());
        mEditTextMqttBrokerAddress.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setMqttBrokerAddress(editable.toString());
            }
        });

        mEditTextMqttBrokerPort = getView().findViewById(R.id.editTextMqttBrokerPort);
        updateMqttBrokerPort(mController.getMqttBrokerPort());
        mEditTextMqttBrokerPort.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setMqttBrokerPort(editable.toString());
            }
        });
    }

    @Override
    public void updateMqttUsername(String username) {
        mEditTextMqttUsername.setText(username);
    }

    @Override
    public void updateMqttPassword(String password) {
        mEditTextMqttPassword.setText(password);
    }

    @Override
    public void updateMqttBrokerAddress(String address) {
        mEditTextMqttBrokerAddress.setText(address);
    }

    @Override
    public void updateMqttBrokerPort(String port) {
        mEditTextMqttBrokerPort.setText(port);
    }

    @Override
    public void setModel(MqttConfigModel model) {
        mController = new MqttConfigController(model, this);
    }

    @Override
    public MqttConfigModel getModel() {
        return mController.getModel();
    }
}
