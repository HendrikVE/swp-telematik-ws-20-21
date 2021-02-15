#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include "HTTPClient.h"
#include "Update.h"
#include "ArduinoLog.h"

class UpdateManager {

public:

    void begin(const char* host, const char* filename);

    int checkForOTAUpdate();

private:

    HTTPClient mHttpClient;

    const char* mHost;
    const char* mFilename;
};

#endif /*UPDATE_MANAGER_H*/
