package de.vanappsteer.windowalarmconfig.controller;

import de.vanappsteer.windowalarmconfig.interfaces.ConfigController;
import de.vanappsteer.windowalarmconfig.interfaces.ConfigView;
import de.vanappsteer.windowalarmconfig.models.WifiConfigModel;

public class WifiConfigController implements ConfigController<WifiConfigModel> {

    private WifiConfigModel mModel;
    private WifiConfigController.View mView;

    public WifiConfigController(WifiConfigModel model, WifiConfigController.View view) {
        mModel = model;
        mView = view;
    }

    @Override
    public void updateView() {
        mView.updateWifiSsid(mModel.getWifiSsid());
        mView.updateWifiPassword(mModel.getWifiPassword());
    }

    @Override
    public WifiConfigModel getModel() {
        return mModel;
    }


    /* BEGIN GETTER */
    public String getSsid() {
        return mModel.getWifiSsid();
    }

    public String getPassword() {
        return mModel.getWifiPassword();
    }
    /* END GETTER */


    /* BEGIN SETTER */
    public void setWifiSsid(String ssid) {
        mModel.setWifiSsid(ssid);
    }

    public void setWifiPassword(String password) {
        mModel.setWifiPassword(password);
    }
    /* END SETTER */


    public interface View extends ConfigView<WifiConfigModel> {

        void updateWifiSsid(String ssid);
        void updateWifiPassword(String password);
    }
}
