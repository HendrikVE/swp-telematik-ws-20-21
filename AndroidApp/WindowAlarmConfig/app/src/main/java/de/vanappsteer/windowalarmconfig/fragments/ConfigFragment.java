package de.vanappsteer.windowalarmconfig.fragments;

import android.support.v4.app.Fragment;

import java.util.Map;
import java.util.UUID;

public abstract class ConfigFragment extends Fragment {

    public class ConfigDescription {

        private UUID uuid;
        private String value;

        public ConfigDescription(UUID uuid, String value) {
            this.uuid = uuid;
            this.value = value;
        }

        public UUID getUuid() {
            return uuid;
        }

        public String getValue() {
            return value;
        }
    }

    public abstract Map<String, ConfigDescription> getInputData();

}
