package de.vanappsteer.windowalarmconfig.fragments;

import android.support.v4.app.Fragment;

import java.util.Map;
import java.util.UUID;

public abstract class ConfigFragment extends Fragment {

    public abstract Map<UUID, String> getInputData();
}
