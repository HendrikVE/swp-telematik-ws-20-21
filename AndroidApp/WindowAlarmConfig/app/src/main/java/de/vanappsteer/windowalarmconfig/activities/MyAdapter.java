package de.vanappsteer.windowalarmconfig.activities;

import android.bluetooth.BluetoothDevice;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import de.vanappsteer.windowalarmconfig.R;

public class MyAdapter extends RecyclerView.Adapter<MyAdapter.MyViewHolder> {

    private List<BluetoothDevice> mDevices;

    public static class MyViewHolder extends RecyclerView.ViewHolder {

        public View mRootView;

        public MyViewHolder(View v) {
            super(v);
            mRootView = v;
        }
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
    public void onBindViewHolder(MyViewHolder holder, int position) {

        String deviceName = mDevices.get(position).getName();
        if (deviceName == null) {
            deviceName = "(name not available)";
        }

        TextView textViewDeviceName = holder.mRootView.findViewById(R.id.textViewDeviceName);
        textViewDeviceName.setText(deviceName);

        TextView textViewDeviceAddress = holder.mRootView.findViewById(R.id.textViewDeviceAddress);
        textViewDeviceAddress.setText(mDevices.get(position).getAddress());

    }

    @Override
    public int getItemCount() {
        return mDevices.size();
    }

    public void setDevices(Set<BluetoothDevice> deviceSet) {

        mDevices.clear();
        mDevices.addAll(deviceSet);
    }
}
