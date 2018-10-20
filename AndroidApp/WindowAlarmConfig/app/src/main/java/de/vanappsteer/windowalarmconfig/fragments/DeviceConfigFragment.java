package de.vanappsteer.windowalarmconfig.fragments;

import android.os.Bundle;
import android.support.design.widget.TextInputEditText;
import android.support.v4.app.Fragment;
import android.text.Editable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import de.vanappsteer.windowalarmconfig.R;
import de.vanappsteer.windowalarmconfig.controller.DeviceConfigController;
import de.vanappsteer.windowalarmconfig.models.DeviceConfigModel;
import de.vanappsteer.windowalarmconfig.util.TextChangeWatcher;

public class DeviceConfigFragment extends Fragment implements DeviceConfigController.View {

    private TextInputEditText mEditTextDeviceRoom;
    private TextInputEditText mEditTextDeviceID;

    private DeviceConfigController mController;

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
    }

    @Override
    public void onDestroyView() {

        mEditTextDeviceRoom = null;
        mEditTextDeviceID = null;

        super.onDestroyView();
    }

    private void initViews() {

        mEditTextDeviceRoom = getView().findViewById(R.id.editTextDeviceRoom);
        updateDeviceRoom(mController.getDeviceRoom());
        mEditTextDeviceRoom.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setDeviceRoom(editable.toString());
            }
        });

        mEditTextDeviceID = getView().findViewById(R.id.editTextDeviceID);
        updateDeviceId(mController.getDeviceId());
        mEditTextDeviceID.addTextChangedListener(new TextChangeWatcher() {

            @Override
            public void afterTextChanged(Editable editable) {
                mController.setDeviceId(editable.toString());
            }
        });
    }

    @Override
    public void updateDeviceRoom(String room) {
        mEditTextDeviceRoom.setText(room);
    }

    @Override
    public void updateDeviceId(String id) {
        mEditTextDeviceID.setText(id);
    }

    @Override
    public void setModel(DeviceConfigModel model) {
        mController = new DeviceConfigController(model, this);
    }

    @Override
    public DeviceConfigModel getModel() {
        return mController.getModel();
    }
}
