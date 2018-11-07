package de.vanappsteer.windowalarmconfig.services;

import android.app.Service;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.UUID;

import de.vanappsteer.windowalarmconfig.util.LoggingUtil;

public class BluetoothDeviceConnectionService extends Service {

    private final UUID BLE_SERVICE_UUID = UUID.fromString("2fa1dab8-3eef-40fc-8540-7fc496a10d75");

    private final IBinder mBinder = new LocalBinder();

    private BluetoothGatt mBluetoothGatt = null;
    private BluetoothGattService mBluetoothGattService = null;
    private boolean mDisconnectPending = false;

    private HashMap<UUID, String> mCharacteristicHashMap;
    private final Queue<BluetoothGattCharacteristic> mReadCharacteristicsOperationsQueue = new LinkedList<>();
    private final Queue<BluetoothGattCharacteristic> mWriteCharacteristicsOperationsQueue = new LinkedList<>();

    private ArrayList<DeviceConnectionListener> mDeviceConnectionListenerList = new ArrayList<>();

    public BluetoothDeviceConnectionService() {

    }

    public class LocalBinder extends Binder {

        public BluetoothDeviceConnectionService getService() {
            return BluetoothDeviceConnectionService.this;
        }
    }

    public static class DeviceConnectionListener {

        public static final int DEVICE_DISCONNECTED = 0;
        public static final int DEVICE_CONNECTION_ERROR_GENERIC = 1;
        public static final int DEVICE_CONNECTION_ERROR_UNSUPPORTED = 2;
        public static final int DEVICE_CONNECTION_ERROR_READ = 3;
        public static final int DEVICE_CONNECTION_ERROR_WRITE = 4;

        public void onDeviceConnected() {}
        public void onDeviceDisconnected() {}
        public void onAllCharacteristicsRead(Map<UUID, String> characteristicMap) {}
        public void onAllCharacteristicsWrote() {}
        public void onDeviceConnectionError(int errorCode) {}
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onDestroy() {
        disconnectDevice();
    }

    public void connectDevice(BluetoothDevice device) {

        LoggingUtil.debug("connectDevice()");

        mBluetoothGatt = device.connectGatt(BluetoothDeviceConnectionService.this, false, mGattCharacteristicCallback);
    }

    public void disconnectDevice() {

        if (mBluetoothGatt != null) {

            synchronized (mReadCharacteristicsOperationsQueue) {
                synchronized (mWriteCharacteristicsOperationsQueue) {
                    int operationsPending = mReadCharacteristicsOperationsQueue.size() + mWriteCharacteristicsOperationsQueue.size();

                    if (operationsPending == 0) {
                        mBluetoothGatt.disconnect();
                        mDisconnectPending = false;
                    }
                    else {
                        mDisconnectPending = true;
                    }
                }
            }
        }
    }

    private boolean isDisconnected() {
        return mBluetoothGatt == null || mBluetoothGattService == null;
    }

    public void readCharacteristics() {

        if (isDisconnected()) {
            for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                listener.onDeviceConnectionError(DeviceConnectionListener.DEVICE_DISCONNECTED);
            }

            return;
        }

        List<BluetoothGattCharacteristic> characteristicList = mBluetoothGattService.getCharacteristics();
        mCharacteristicHashMap = new HashMap<>();

        mReadCharacteristicsOperationsQueue.addAll(characteristicList);

        // initial call of readCharacteristic, further calls are done within onCharacteristicRead afterwards
        boolean success = mBluetoothGatt.readCharacteristic(mReadCharacteristicsOperationsQueue.poll());
        if (! success) {
            for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                listener.onDeviceConnectionError(DeviceConnectionListener.DEVICE_CONNECTION_ERROR_READ);
            }
        }
    }

    public void writeCharacteristics(Map<UUID, String> characteristicMap) {

        if (isDisconnected()) {
            for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                listener.onDeviceConnectionError(DeviceConnectionListener.DEVICE_DISCONNECTED);
            }

            return;
        }

        synchronized (mWriteCharacteristicsOperationsQueue) {

            boolean needInitialCall = mWriteCharacteristicsOperationsQueue.size() == 0;

            BluetoothGattService gattService = mBluetoothGatt.getService(BLE_SERVICE_UUID);

            for (Map.Entry<UUID, String> entry : characteristicMap.entrySet()) {

                BluetoothGattCharacteristic characteristic = gattService.getCharacteristic(entry.getKey());

                characteristic.setValue(entry.getValue());

                LoggingUtil.debug("entry.getValue() = " + entry.getValue());
                LoggingUtil.debug("characteristic.getStringValue(0) = " +  characteristic.getStringValue(0));

                mWriteCharacteristicsOperationsQueue.add(characteristic);
            }

            if (needInitialCall) {
                // initial call of writeCharacteristic, further calls are done within onCharacteristicWrite afterwards
                boolean success = mBluetoothGatt.writeCharacteristic(mWriteCharacteristicsOperationsQueue.poll());
                if (! success) {
                    for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                        listener.onDeviceConnectionError(DeviceConnectionListener.DEVICE_CONNECTION_ERROR_WRITE);
                    }
                }
            }
        }
    }

    public void addDeviceConnectionListener(DeviceConnectionListener listener) {
        mDeviceConnectionListenerList.add(listener);
    }

    public void removeDeviceConnectionListener(DeviceConnectionListener listener) {
        mDeviceConnectionListenerList.remove(listener);
    }

    private BluetoothGattCallback mGattCharacteristicCallback = new BluetoothGattCallback() {

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {

            LoggingUtil.debug("onConnectionStateChange");
            LoggingUtil.debug("status: " + status);
            LoggingUtil.debug("newState: " + newState);

            if (status == BluetoothGatt.GATT_SUCCESS) {

                switch (newState) {

                    case BluetoothProfile.STATE_CONNECTED:
                        LoggingUtil.debug("discoverServices");
                        gatt.discoverServices();
                        break;

                    case BluetoothProfile.STATE_DISCONNECTED:
                        gatt.close();
                        mBluetoothGatt = null;
                        mBluetoothGattService = null;
                        for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                            listener.onDeviceDisconnected();
                        }
                        break;

                    default:
                        LoggingUtil.debug("unhandled state: " + newState);
                }
            }
            else {
                for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                    listener.onDeviceConnectionError(DeviceConnectionListener.DEVICE_CONNECTION_ERROR_GENERIC);
                }
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {

            LoggingUtil.debug("onServicesDiscovered");

            if (status != BluetoothGatt.GATT_SUCCESS) {
                LoggingUtil.error("status != BluetoothGatt.GATT_SUCCESS");

                for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                    listener.onDeviceConnectionError(DeviceConnectionListener.DEVICE_CONNECTION_ERROR_GENERIC);
                }

                return;
            }

            mBluetoothGattService = gatt.getService(BLE_SERVICE_UUID);
            if (mBluetoothGattService == null) {

                gatt.disconnect();

                for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                    listener.onDeviceConnectionError(DeviceConnectionListener.DEVICE_CONNECTION_ERROR_UNSUPPORTED);
                }

                return;
            }

            for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                listener.onDeviceConnected();
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {

            LoggingUtil.debug("onCharacteristicRead");
            LoggingUtil.debug("uuid: " + characteristic.getUuid());
            LoggingUtil.debug("value: " + characteristic.getStringValue(0));

            mCharacteristicHashMap.put(characteristic.getUuid(), characteristic.getStringValue(0));

            synchronized (mReadCharacteristicsOperationsQueue) {

                if (mReadCharacteristicsOperationsQueue.size() > 0) {
                    boolean success = gatt.readCharacteristic(mReadCharacteristicsOperationsQueue.poll());
                    if (! success) {
                        for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                            listener.onDeviceConnectionError(DeviceConnectionListener.DEVICE_CONNECTION_ERROR_READ);
                        }
                    }
                }
                else {
                    for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                        listener.onAllCharacteristicsRead(mCharacteristicHashMap);
                    }
                }
            }
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {

            LoggingUtil.debug("onCharacteristicWrite");
            LoggingUtil.debug("uuid: " + characteristic.getUuid());
            LoggingUtil.debug("value: " + characteristic.getStringValue(0));

            synchronized (mWriteCharacteristicsOperationsQueue) {

                if (mWriteCharacteristicsOperationsQueue.size() > 0) {
                    boolean success = gatt.writeCharacteristic(mWriteCharacteristicsOperationsQueue.poll());
                    if (success) {
                        for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                            listener.onAllCharacteristicsWrote();
                        }
                    }
                    else {
                        for (DeviceConnectionListener listener : mDeviceConnectionListenerList) {
                            listener.onDeviceConnectionError(DeviceConnectionListener.DEVICE_CONNECTION_ERROR_WRITE);
                        }
                    }
                }
                else if (mDisconnectPending) {
                    disconnectDevice();
                }
            }
        }
    };
}
