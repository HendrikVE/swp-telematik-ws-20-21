#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include "HTTPClient.h"
#include "Update.h"
#include "ArduinoLog.h"

class UpdateManager {

public:

    void begin(const char* host, const char* filename, const char* user, const char* password, const char* deviceID);

    int checkForOTAUpdate();

private:

    HTTPClient mHttpClient;

    const char* mHost;
    const char* mFilename;
    const char* mUser;
    const char* mPassword;
    const char* mDeviceID;
};

#endif /*UPDATE_MANAGER_H*/
