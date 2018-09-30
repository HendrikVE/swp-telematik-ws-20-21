package de.vanappsteer.windowalarmconfig.activities;

import android.bluetooth.BluetoothDevice;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Set;

import de.vanappsteer.windowalarmconfig.R;

public class MyAdapter extends RecyclerView.Adapter<MyAdapter.MyViewHolder> {

    private List<BluetoothDevice> mDevices;
    private OnDeviceSelectionListener mOnDeviceSelectionListener;

    static class MyViewHolder extends RecyclerView.ViewHolder {

        private View mRootView;

        MyViewHolder(View v) {
            super(v);
            mRootView = v;
        }

        View getRootView() {
            return mRootView;
        }
    }

    static abstract class OnDeviceSelectionListener {

        abstract void onDeviceSelected(BluetoothDevice device);
    }

    public MyAdapter() {
        mDevices = new ArrayList<>();
    }

    public MyAdapter(List<BluetoothDevice> myDataset) {
        mDevices = myDataset;
    }

    @Override
    public MyAdapter.MyViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {

        View rootView = LayoutInflater.from(parent.getContext()).inflate(R.layout.list_item, parent, false);

        MyViewHolder vh = new MyViewHolder(rootView);

        return vh;
    }

    @Override
    public void onBindViewHolder(MyViewHolder holder, final int position) {

        holder.getRootView().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mOnDeviceSelectionListener.onDeviceSelected(mDevices.get(position));
            }
        });

        String deviceName = mDevices.get(position).getName();
        if (deviceName == null) {
            deviceName = "(name not available)";
        }

        TextView textViewDeviceName = holder.getRootView().findViewById(R.id.textViewDeviceName);
        textViewDeviceName.setText(deviceName);

        TextView textViewDeviceAddress = holder.getRootView().findViewById(R.id.textViewDeviceAddress);
        textViewDeviceAddress.setText(mDevices.get(position).getAddress());

    }

    @Override
    public int getItemCount() {
        return mDevices.size();
    }

    public void setDevices(Set<BluetoothDevice> deviceSet) {

        mDevices.clear();
        mDevices.addAll(deviceSet);

        Collections.sort(mDevices, new DeviceComparator());
    }

    public void setOnDeviceSelectionListener(OnDeviceSelectionListener listener) {
        mOnDeviceSelectionListener = listener;
    }

    private class DeviceComparator implements Comparator<BluetoothDevice> {

        public int compare(BluetoothDevice first, BluetoothDevice second) {

            if (first.getName() == null) {
                return 1;
            }

            if (second.getName() == null) {
                return -1;
            }

            if (first.getName() == null && second.getName() == null) {
                return 0;
            }

            return first.getName().compareTo(second.getName());
        }
    }
}
