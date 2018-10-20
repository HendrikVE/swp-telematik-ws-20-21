package de.vanappsteer.windowalarmconfig.fragments;

import android.os.Bundle;
import android.support.design.widget.TextInputEditText;
import android.support.v4.app.Fragment;
import android.text.Editable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import de.vanappsteer.windowalarmconfig.R;
import de.vanappsteer.windowalarmconfig.controller.OtaConfigController;
import de.vanappsteer.windowalarmconfig.models.OtaConfigModel;
import de.vanappsteer.windowalarmconfig.util.TextChangeWatcher;

public class OtaConfigFragment extends Fragment implements OtaConfigController.View {

    private TextInputEditText mEditTextOtaServerAddress;
    private TextInputEditText mEditTextOtaFilename;
    private TextInputEditText mEditTextOtaUsername;
    private TextInputEditText mEditTextOtaPassword;

    private OtaConfigController mController;

    public OtaConfigFragment() {
        // Required empty public constructor
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        return inflater.inflate(R.layout.fragment_ota_config, container, false);
    }

    @Override
    public void onActivityCreated (Bundle savedInstanceState) {

        super.onActivityCreated(savedInstanceState);

        initViews();
    }

    @Override
    public void onDestroyView() {

        mEditTextOtaServerAddress = null;
        mEditTextOtaFilename = null;
        mEditTextOtaUsername = null;
        mEditTextOtaPassword = null;

        super.onDestroyView();
    }

    private void initViews() {
        mEditTextOtaServerAddress = getView().findViewById(R.id.editTextOtaServerAddress);
        updateOtaServerAddress(mController.getOtaServerAddress());
        mEditTextOtaServerAddress.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setOtaServerAddress(editable.toString());
            }
        });

        mEditTextOtaFilename = getView().findViewById(R.id.editTextOtaFilename);
        updateOtaFilename(mController.getOtaFilename());
        mEditTextOtaFilename.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setOtaFilename(editable.toString());
            }
        });

        mEditTextOtaUsername = getView().findViewById(R.id.editTextOtaUsername);
        updateOtaUsername(mController.getOtaUsername());
        mEditTextOtaUsername.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setOtaUsername(editable.toString());
            }
        });

        mEditTextOtaPassword = getView().findViewById(R.id.editTextOtaPassword);
        updateOtaPassword(mController.getOtaPassword());
        mEditTextOtaPassword.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setOtaPassword(editable.toString());
            }
        });
    }

    @Override
    public void updateOtaServerAddress(String address) {
        mEditTextOtaServerAddress.setText(address);
    }

    @Override
    public void updateOtaFilename(String filename) {
        mEditTextOtaFilename.setText(filename);
    }

    @Override
    public void updateOtaUsername(String username) {
        mEditTextOtaUsername.setText(username);
    }

    @Override
    public void updateOtaPassword(String password) {
        mEditTextOtaPassword.setText(password);
    }

    @Override
    public void setModel(OtaConfigModel model) {
        mController = new OtaConfigController(model, this);
    }

    @Override
    public OtaConfigModel getModel() {
        return mController.getModel();
    }
}
