package de.vanappsteer.windowalarmconfig.controller;

import de.vanappsteer.windowalarmconfig.interfaces.ConfigController;
import de.vanappsteer.windowalarmconfig.interfaces.ConfigView;
import de.vanappsteer.windowalarmconfig.models.MqttConfigModel;

public class MqttConfigController implements ConfigController<MqttConfigModel> {

    private MqttConfigModel mModel;
    private MqttConfigController.View mView;

    public MqttConfigController(MqttConfigModel model, MqttConfigController.View view) {
        mModel = model;
        mView = view;
    }

    @Override
    public void updateView() {
        mView.updateMqttUsername(mModel.getMqttUsername());
        mView.updateMqttPassword(mModel.getMqttUsername());
        mView.updateMqttBrokerAddress(mModel.getMqttBrokerAddress());
        mView.updateMqttBrokerPort(mModel.getMqttBrokerPort());
    }

    @Override
    public MqttConfigModel getModel() {
        return mModel;
    }


    /* BEGIN GETTER */
    public String getMqttUsername() {
        return mModel.getMqttUsername();
    }

    public String getMqttPassword() {
        return mModel.getMqttPassword();
    }

    public String getMqttBrokerAddress() {
        return mModel.getMqttBrokerAddress();
    }

    public String getMqttBrokerPort() {
        return mModel.getMqttBrokerPort();
    }
    /* END GETTER */


    /* BEGIN SETTER */
    public void setMqttUsername(String username) {
        mModel.setMqttUsername(username);
    }

    public void setMqttPassword(String password) {
        mModel.setMqttPassword(password);
    }

    public void setMqttBrokerAddress(String address) {
        mModel.setMqttBrokerAddress(address);
    }

    public void setMqttBrokerPort(String port) {
        mModel.setMqttBrokerPort(port);
    }
    /* END SETTER */


    public interface View extends ConfigView<MqttConfigModel> {

        void updateMqttUsername(String username);
        void updateMqttPassword(String password);
        void updateMqttBrokerAddress(String address);
        void updateMqttBrokerPort(String port);
    }
}
