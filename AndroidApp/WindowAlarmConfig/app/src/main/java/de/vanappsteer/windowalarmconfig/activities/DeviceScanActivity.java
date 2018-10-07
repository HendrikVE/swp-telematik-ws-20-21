package de.vanappsteer.windowalarmconfig.activities;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PersistableBundle;
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
import android.view.MenuItem;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;
import java.util.UUID;

import de.vanappsteer.windowalarmconfig.adapter.DeviceListAdapter;
import de.vanappsteer.windowalarmconfig.R;
import de.vanappsteer.windowalarmconfig.util.LoggingUtil;

public class DeviceScanActivity extends AppCompatActivity {

    private final UUID BLE_SERVICE_UUID = UUID.fromString("2fa1dab8-3eef-40fc-8540-7fc496a10d75");

    private final int ACTIVITY_RESULT_ENABLE_BLUETOOTH = 1;
    private final int ACTIVITY_RESULT_ENABLE_LOCATION_PERMISSION = 2;

    private final int REQUEST_PERMISSION_COARSE_LOCATION = 1;

    private final int COMMAND_SHOW_CONNECTION_ERROR_DIALOG = 1;
    private final int COMMAND_SHOW_DEVICE_UNSUPPORTED_DIALOG = 2;

    private final String KEY_SP_ASKED_FOR_LOCATION = "KEY_SP_ASKED_FOR_LOCATION";

    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;

    private Set<BluetoothDevice> bleDeviceSet = new HashSet<>();

    private DeviceListAdapter mAdapter;
    private boolean mScanSwitchEnabled = true;
    private boolean mIsScanning = false;
    private boolean mScanPaused = false;

    private SwitchCompat mScanSwitch;
    private ProgressBar mScanProgressbar;
    private TextView mTextViewEnableBluetooth;

    private BluetoothGatt mConnectedBluetoothGatt = null;

    private AlertDialog mDialogConnectDevice;

    private SharedPreferences mSP;

    Handler mUiHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(Message message) {

            AlertDialog.Builder builder;
            AlertDialog dialog;

            switch (message.what) {

                case COMMAND_SHOW_CONNECTION_ERROR_DIALOG:
                    builder = new AlertDialog.Builder(DeviceScanActivity.this);
                    builder.setTitle(R.string.dialog_bluetooth_device_connection_error_title);
                    builder.setMessage(R.string.dialog_bluetooth_device_connection_error_message);
                    builder.setPositiveButton(R.string.button_ok, null);

                    dialog = builder.create();
                    dialog.show();
                    break;

                case COMMAND_SHOW_DEVICE_UNSUPPORTED_DIALOG:
                    builder = new AlertDialog.Builder(DeviceScanActivity.this);
                    builder.setTitle(R.string.dialog_bluetooth_device_not_supported_title);
                    builder.setMessage(R.string.dialog_bluetooth_device_not_supported_message);
                    builder.setPositiveButton(R.string.button_ok, null);

                    dialog = builder.create();
                    dialog.show();
                    break;

                default:
                    LoggingUtil.warning("unhandled command: " + message.what);
            }
        }
    };

    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();

            if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
                final int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
                switch (state) {

                    case BluetoothAdapter.STATE_OFF:
                        stopScan();
                        break;

                    case BluetoothAdapter.STATE_TURNING_OFF:
                        // do nothing
                        break;

                    case BluetoothAdapter.STATE_ON:
                        // do nothing
                        break;

                    case BluetoothAdapter.STATE_TURNING_ON:
                        // do nothing
                        break;

                    default:
                        LoggingUtil.warning("unhandled state: " + state);
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_scan);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        initViews();

        mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mSP = PreferenceManager.getDefaultSharedPreferences(this);

        IntentFilter filter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
        registerReceiver(mBroadcastReceiver, filter);
    }

    @Override
    protected void onPause() {

        super.onPause();

        if (mIsScanning) {
            mScanPaused = true;
        }
        stopScan();
    }

    @Override
    protected void onResume() {

        super.onResume();

        if (mScanSwitch != null && (mScanSwitch.isChecked()) || mScanPaused) {
            checkPermissions();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if (mConnectedBluetoothGatt != null) {
            mConnectedBluetoothGatt.disconnect();
            mConnectedBluetoothGatt.close();
        }

        unregisterReceiver(mBroadcastReceiver);
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
                LoggingUtil.warning("unhandled request code: " + requestCode);
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

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        switch (item.getItemId()) {

            case R.id.menuItemBluetoothSwitch:
                return true;

            case R.id.menuItemAbout:
                Intent intent = new Intent(DeviceScanActivity.this, AboutApp.class);
                startActivity(intent);
                return true;

            default:
                return super.onOptionsItemSelected(item);
        }
    }

    private void startScan() {

        mScanPaused = false;

        mScanProgressbar.setVisibility(View.VISIBLE);

        mScanSwitchEnabled = false;
        mScanSwitch.setChecked(true);
        mScanSwitchEnabled = true;

        mTextViewEnableBluetooth.setVisibility(View.GONE);

        mIsScanning = true;
        mBluetoothAdapter.startLeScan(mLeScanCallback);
    }

    private void stopScan() {

        mScanProgressbar.setVisibility(View.INVISIBLE);

        mScanSwitchEnabled = false;
        mScanSwitch.setChecked(false);
        mScanSwitchEnabled = true;

        // mBluetoothAdapter is null if only checkPermissions() was called, but not checkBluetooth()
        if (mBluetoothAdapter != null) {
            mIsScanning = false;
            mBluetoothAdapter.stopLeScan(mLeScanCallback);
        }

        bleDeviceSet.clear();
        mAdapter.setDevices(bleDeviceSet);
        mAdapter.notifyDataSetChanged();

        mTextViewEnableBluetooth.setVisibility(View.VISIBLE);
    }

    private void initViews() {
        initRecyclerView();
        mScanProgressbar = findViewById(R.id.scanProgressbar);
        mTextViewEnableBluetooth = findViewById(R.id.textViewEnableBluetooth);
    }

    private void initRecyclerView() {

        RecyclerView recyclerView = findViewById(R.id.recyclerView);
        recyclerView.setHasFixedSize(true);

        RecyclerView.LayoutManager mLayoutManager = new LinearLayoutManager(this);
        recyclerView.setLayoutManager(mLayoutManager);

        mAdapter = new DeviceListAdapter();
        mAdapter.setOnDeviceSelectionListener(new DeviceListAdapter.OnDeviceSelectionListener() {
            @Override
            public void onDeviceSelected(BluetoothDevice device) {

                mConnectedBluetoothGatt = device.connectGatt(DeviceScanActivity.this, false, mGattCallback);

                AlertDialog.Builder builder = new AlertDialog.Builder(DeviceScanActivity.this);
                builder.setTitle(R.string.dialog_bluetooth_device_connecting_title);
                builder.setView(R.layout.progress_infinite);
                builder.setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        mConnectedBluetoothGatt.disconnect();
                        mConnectedBluetoothGatt.close();
                    }
                });
                builder.setCancelable(false);

                mDialogConnectDevice = builder.create();
                mDialogConnectDevice.show();
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
            boolean alreadyAskedBefore = mSP.getBoolean(KEY_SP_ASKED_FOR_LOCATION, false);

            if ( !showRationale && alreadyAskedBefore) {// user CHECKED "never ask again"

                showLocationRequestDialog(true);
            }
            else {
                showLocationRequestDialog(false);

                SharedPreferences.Editor editor = mSP.edit();
                editor.putBoolean(KEY_SP_ASKED_FOR_LOCATION, true);
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

                    if (! mIsScanning) {
                        return;
                    }

                    boolean added = false;

                    // update device in set if a name was found after address was already discovered
                    BluetoothDevice deviceFound = getDeviceByBleAddress(bleDeviceSet, device.getAddress());
                    if (deviceFound != null && deviceFound.getName() == null && device.getName() != null) {
                        bleDeviceSet.remove(deviceFound);
                        added = bleDeviceSet.add(device);
                    }
                    else if(deviceFound == null) {
                        added = bleDeviceSet.add(device);
                    }

                    if (added) {
                        mAdapter.setDevices(bleDeviceSet);
                        mAdapter.notifyDataSetChanged();
                    }
                }
            });
        }
    };

    private BluetoothDevice getDeviceByBleAddress(Set<BluetoothDevice> deviceSet, String address) {

        for (BluetoothDevice device : deviceSet) {
            if (device.getAddress().equals(address)) {
                return device;
            }
        }

        return null;
    }

    private BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {

            LoggingUtil.debug("onConnectionStateChange");
            LoggingUtil.debug("status: " + status);
            LoggingUtil.debug("newState: " + newState);

            if (newState == BluetoothProfile.STATE_CONNECTED) {
                LoggingUtil.debug("discoverServices");
                gatt.discoverServices();
            }
            else {
                mDialogConnectDevice.dismiss();

                mConnectedBluetoothGatt.disconnect();
                mConnectedBluetoothGatt.close();

                Message message = mUiHandler.obtainMessage(COMMAND_SHOW_CONNECTION_ERROR_DIALOG, null);
                message.sendToTarget();
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {

            LoggingUtil.debug("onServicesDiscovered");
            mDialogConnectDevice.dismiss();

            if (status != BluetoothGatt.GATT_SUCCESS) {
                // TODO: Handle the error
                LoggingUtil.error("status != BluetoothGatt.GATT_SUCCESS");
                return;
            }

            BluetoothGattService gattService = gatt.getService(BLE_SERVICE_UUID);
            if (gattService == null) {
                mConnectedBluetoothGatt.disconnect();
                mConnectedBluetoothGatt.close();

                Message message = mUiHandler.obtainMessage(COMMAND_SHOW_DEVICE_UNSUPPORTED_DIALOG, null);
                message.sendToTarget();

                return;
            }

            List<BluetoothGattCharacteristic> characteristicList = gattService.getCharacteristics();
            HashMap<UUID, String> characteristicHashMap = new HashMap<>();

            for (BluetoothGattCharacteristic characteristic : characteristicList) {

                if (characteristic.getValue() == null) {
                    characteristic.setValue("" + new Random().nextInt(100 + 1));
                }
                characteristicHashMap.put(characteristic.getUuid(), characteristic.getStringValue(0));
            }

            Intent intent = new Intent(DeviceScanActivity.this, DeviceConfigActivity.class);
            intent.putExtra(DeviceConfigActivity.KEY_CHARACTERISTIC_HASH_MAP, characteristicHashMap);
            startActivity(intent);
        }
    };

}
