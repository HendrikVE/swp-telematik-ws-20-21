package de.vanappsteer.windowalarmconfig.activities;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.SwitchCompat;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.ProgressBar;

import java.util.HashSet;
import java.util.Set;
import java.util.UUID;

import de.vanappsteer.windowalarmconfig.R;
import de.vanappsteer.windowalarmconfig.util.LoggingUtil;

public class DeviceScanActivity extends AppCompatActivity {

    private final UUID BLE_SERVICE_UUID = UUID.fromString("2fa1dab8-3eef-40fc-8540-7fc496a10d75");

    private final UUID BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID = UUID.fromString("d3491796-683b-4b9c-aafb-f51a35459d43");
    private final UUID BLE_CHARACTERISTIC_CONFIG_DEVICE_ID_UUID = UUID.fromString("4745e11f-b403-4cfb-83bb-710d46897875");

    private final UUID BLE_CHARACTERISTIC_CONFIG_OTA_HOST_UUID = UUID.fromString("2f44b103-444c-48f5-bf60-91b81dfa0a25");
    private final UUID BLE_CHARACTERISTIC_CONFIG_OTA_FILENAME_UUID = UUID.fromString("4b95d245-db08-4c56-98f9-738faa8cfbb6");
    private final UUID BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_USERNAME_UUID = UUID.fromString("1c93dce2-3796-4027-9f55-6d251c41dd34");
    private final UUID BLE_CHARACTERISTIC_CONFIG_OTA_SERVER_PASSWORD_UUID = UUID.fromString("0e837309-5336-45a3-9b69-d0f7134f30ff");

    private final UUID BLE_CHARACTERISTIC_CONFIG_WIFI_SSID_UUID = UUID.fromString("8ca0bf1d-bb5d-4a66-9191-341fd805e288");
    private final UUID BLE_CHARACTERISTIC_CONFIG_WIFI_PASSWORD_UUID = UUID.fromString("fa41c195-ae99-422e-8f1f-0730702b3fc5");

    private final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_USER_UUID = UUID.fromString("69150609-18f8-4523-a41f-6d9a01d2e08d");
    private final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_PASSWORD_UUID = UUID.fromString("8bebec77-ea21-4c14-9d64-dbec1fd5267c");
    private final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_IP_UUID = UUID.fromString("e3b150fb-90a2-4cd3-80c5-b1189e110ef1");
    private final UUID BLE_CHARACTERISTIC_CONFIG_MQTT_SERVER_PORT_UUID = UUID.fromString("4eeff953-0f5e-43ee-b1be-1783a8190b0d");

    private final UUID BLE_CHARACTERISTIC_CONFIG_SENSOR_POLL_INTERVAL_MS_UUID = UUID.fromString("68011c92-854a-4f2c-a94c-5ee37dc607c3");

    private final int ACTIVITY_RESULT_ENABLE_BLUETOOTH = 1;
    private final int ACTIVITY_RESULT_ENABLE_LOCATION_PERMISSION = 2;

    private final int REQUEST_PERMISSION_COARSE_LOCATION = 1;

    private final String SHARED_PREFERENCES_ASKED_FOR_LOCATION = "SHARED_PREFERENCES_ASKED_FOR_LOCATION";

    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;

    private Set<BluetoothDevice> bleDeviceSet = new HashSet<>();

    private MyAdapter mAdapter;
    private boolean mScanSwitchEnabled = true;

    private RecyclerView.LayoutManager mLayoutManager;
    private SwitchCompat mScanSwitch;
    private ProgressBar mScanProgressbar;

    private BluetoothGatt mConnectedBluetoothGatt = null;

    private SharedPreferences mSP;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_scan);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        initViews();

        mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mSP = PreferenceManager.getDefaultSharedPreferences(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        stopScan();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mScanSwitch != null && mScanSwitch.isChecked()) {
            checkPermissions();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if (mConnectedBluetoothGatt != null) {
            mConnectedBluetoothGatt.disconnect();
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {

        if (requestCode == ACTIVITY_RESULT_ENABLE_BLUETOOTH) {
            if (resultCode == RESULT_OK) {
                checkPermissions();
            }
            else {
                mScanSwitch.setChecked(false);
            }
        }
        else if (requestCode == ACTIVITY_RESULT_ENABLE_LOCATION_PERMISSION) {
            checkPermissions();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {

        switch (requestCode) {

            case REQUEST_PERMISSION_COARSE_LOCATION: {

                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    checkBluetooth();
                }
                else {
                    checkPermissions();
                }

                break;
            }

            default:
                LoggingUtil.warning("unknown request code: " + requestCode);
                break;
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {

        super.onCreateOptionsMenu(menu);

        getMenuInflater().inflate(R.menu.scan_menu, menu);

        mScanSwitch = menu.findItem(R.id.menuItemBluetoothSwitch).getActionView().findViewById(R.id.bluetoothScanSwitch);
        mScanSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean isChecked) {

                if (! mScanSwitchEnabled) {
                    return;
                }

                if (isChecked) {
                    checkPermissions();
                }
                else {
                    stopScan();
                }
            }
        });

        checkPermissions();

        return true;
    }

    private void startScan() {

        mScanProgressbar.setVisibility(View.VISIBLE);

        mScanSwitchEnabled = false;
        mScanSwitch.setChecked(true);
        mScanSwitchEnabled = true;

        mBluetoothAdapter.startLeScan(mLeScanCallback);
    }

    private void stopScan() {

        mScanProgressbar.setVisibility(View.INVISIBLE);

        // mBluetoothAdapter is null if only checkPermissions was called, but not checkBluetooth
        if (mBluetoothAdapter != null) {
            mBluetoothAdapter.stopLeScan(mLeScanCallback);
        }

        bleDeviceSet.clear();
        mAdapter.setDevices(bleDeviceSet);
        mAdapter.notifyDataSetChanged();
    }

    private void initViews() {
        initRecyclerView();
        mScanProgressbar = findViewById(R.id.scanProgressbar);
    }

    private void initRecyclerView() {

        RecyclerView recyclerView = findViewById(R.id.recyclerView);
        recyclerView.setHasFixedSize(true);

        mLayoutManager = new LinearLayoutManager(this);
        recyclerView.setLayoutManager(mLayoutManager);

        mAdapter = new MyAdapter();
        mAdapter.setOnDeviceSelectionListener(new MyAdapter.OnDeviceSelectionListener() {
            @Override
            void onDeviceSelected(BluetoothDevice device) {
                mConnectedBluetoothGatt = device.connectGatt(DeviceScanActivity.this, false, mGattCallback);
            }
        });
        recyclerView.setAdapter(mAdapter);
    }

    private void checkBluetooth() {

        if (mBluetoothManager != null) {
            mBluetoothAdapter = mBluetoothManager.getAdapter();
        }

        if (mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, ACTIVITY_RESULT_ENABLE_BLUETOOTH);
        }
        else {
            startScan();
        }
    }

    private void checkPermissions() {

        boolean coarseLocationGranted = ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED;
        if (! coarseLocationGranted) {

            boolean showRationale = ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.ACCESS_COARSE_LOCATION);
            boolean alreadyAskedBefore = mSP.getBoolean(SHARED_PREFERENCES_ASKED_FOR_LOCATION, false);

            if ( !showRationale && alreadyAskedBefore) {// user CHECKED "never ask again"

                showLocationRequestDialog(true);
            }
            else {
                showLocationRequestDialog(false);

                SharedPreferences.Editor editor = mSP.edit();
                editor.putBoolean(SHARED_PREFERENCES_ASKED_FOR_LOCATION, true);
                editor.apply();
            }
        }
        else {
            checkBluetooth();
        }
    }

    private void showLocationRequestDialog(boolean neverAskAgain) {

        AlertDialog.Builder builder = new AlertDialog.Builder(this);

        if (neverAskAgain) {
            builder.setMessage(R.string.dialog_coarse_location_permitted_message).setTitle(R.string.dialog_coarse_location_permitted_title);
            builder.setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    Intent intent = new Intent();
                    intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                    Uri uri = Uri.fromParts("package", getPackageName(), null);
                    intent.setData(uri);
                    startActivityForResult(intent, ACTIVITY_RESULT_ENABLE_LOCATION_PERMISSION);
                }
            });
        }
        else {
            builder.setMessage(R.string.dialog_request_coarse_location_message).setTitle(R.string.dialog_request_coarse_location_title);
            builder.setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    ActivityCompat.requestPermissions(DeviceScanActivity.this, new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, REQUEST_PERMISSION_COARSE_LOCATION);
                }
            });
        }

        builder.setCancelable(false);

        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private BluetoothAdapter.LeScanCallback mLeScanCallback = new BluetoothAdapter.LeScanCallback() {

        @Override
        public void onLeScan(final BluetoothDevice device, int rssi, byte[] scanRecord) {

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    boolean added = bleDeviceSet.add(device);
                    if (added) {
                        mAdapter.setDevices(bleDeviceSet);
                        mAdapter.notifyDataSetChanged();
                    }
                }
            });
        }
    };

    private BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {

            LoggingUtil.debug("onConnectionStateChange");

            if (newState == BluetoothProfile.STATE_CONNECTED) {
                LoggingUtil.debug("discoverServices");
                gatt.discoverServices();
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {

            LoggingUtil.debug("onServicesDiscovered");

            if (status != BluetoothGatt.GATT_SUCCESS) {
                // TODO: Handle the error
                LoggingUtil.error("status != BluetoothGatt.GATT_SUCCESS");
                return;
            }

            BluetoothGattCharacteristic characteristic = gatt
                    .getService(BLE_SERVICE_UUID)
                    .getCharacteristic(BLE_CHARACTERISTIC_CONFIG_DEVICE_ROOM_UUID);

            characteristic.setValue("hallo");
            gatt.writeCharacteristic(characteristic);
        }
    };

}
