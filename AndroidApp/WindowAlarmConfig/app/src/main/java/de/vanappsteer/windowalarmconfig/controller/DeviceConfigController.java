package de.vanappsteer.windowalarmconfig.controller;

import de.vanappsteer.windowalarmconfig.interfaces.ConfigController;
import de.vanappsteer.windowalarmconfig.interfaces.ConfigView;
import de.vanappsteer.windowalarmconfig.interfaces.DeviceConfigView;
import de.vanappsteer.windowalarmconfig.models.DeviceConfigModel;

public class DeviceConfigController implements ConfigController<DeviceConfigModel> {

    private DeviceConfigModel mModel;
    private DeviceConfigView mView;

    public DeviceConfigController(DeviceConfigModel model, DeviceConfigView view) {
        mModel = model;
        mView = view;
    }

    @Override
    public void updateView() {
        mView.updateDeviceRoom(mModel.getDeviceRoom());
        mView.updateDeviceId(mModel.getDeviceId());
    }

    @Override
    public DeviceConfigModel getModel() {
        return mModel;
    }


    /* BEGIN GETTER */
    public String getDeviceRoom() {
        return mModel.getDeviceRoom();
    }

    public String getDeviceId() {
        return mModel.getDeviceId();
    }
    /* END GETTER */


    /* BEGIN SETTER */
    public void setDeviceRoom(String room) {
        mModel.setDeviceRoom(room);
    }

    public void setDeviceId(String id) {
        mModel.setDeviceId(id);
    }
    /* END SETTER */
}
