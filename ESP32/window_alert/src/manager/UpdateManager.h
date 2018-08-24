#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include "HTTPClient.h"
#include "Update.h"
#include "ArduinoLog.h"

class UpdateManager {

public:

    void begin();

    void checkForOTAUpdate();

private:

    HTTPClient mHttpClient;

    Logging logger;

    static void printTag(Print* _logOutput) {
        char c[12];
        sprintf(c, "%s ", "[UpdateManager] ");
        _logOutput->print(c);
    }

    static void printNewline(Print* _logOutput) {
        _logOutput->print("\n");
    }

};

#endif /*UPDATE_MANAGER_H*/
