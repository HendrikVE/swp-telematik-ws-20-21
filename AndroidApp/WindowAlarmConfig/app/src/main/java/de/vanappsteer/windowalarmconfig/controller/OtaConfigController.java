package de.vanappsteer.windowalarmconfig.controller;

import de.vanappsteer.windowalarmconfig.interfaces.ConfigController;
import de.vanappsteer.windowalarmconfig.interfaces.ConfigView;
import de.vanappsteer.windowalarmconfig.models.OtaConfigModel;

public class OtaConfigController implements ConfigController<OtaConfigModel> {

    private OtaConfigModel mModel;
    private OtaConfigController.View mView;

    public OtaConfigController(OtaConfigModel model, OtaConfigController.View view) {
        mModel = model;
        mView = view;
    }

    @Override
    public void updateView() {
        mView.updateOtaServerAddress(mModel.getOtaServerAddress());
        mView.updateOtaFilename(mModel.getOtaFilename());
        mView.updateOtaUsername(mModel.getOtaUsername());
        mView.updateOtaPassword(mModel.getOtaPassword());
    }

    @Override
    public OtaConfigModel getModel() {
        return mModel;
    }


    /* BEGIN GETTER */
    public String getOtaServerAddress() {
        return mModel.getOtaServerAddress();
    }

    public String getOtaFilename() {
        return mModel.getOtaFilename();
    }

    public String getOtaUsername() {
        return mModel.getOtaUsername();
    }

    public String getOtaPassword() {
        return mModel.getOtaPassword();
    }
    /* END GETTER */


    /* BEGIN SETTER */
    public void setOtaServerAddress(String serverAddress) {
        mModel.setOtaServerAddress(serverAddress);
    }

    public void setOtaFilename(String filename) {
        mModel.setOtaFilename(filename);
    }

    public void setOtaUsername(String username) {
        mModel.setOtaUsername(username);
    }

    public void setOtaPassword(String password) {
        mModel.setOtaPassword(password);
    }
    /* END SETTER */


    public interface View extends ConfigView<OtaConfigModel> {

        void updateOtaServerAddress(String address);
        void updateOtaFilename(String filename);
        void updateOtaUsername(String username);
        void updateOtaPassword(String password);
    }
}
