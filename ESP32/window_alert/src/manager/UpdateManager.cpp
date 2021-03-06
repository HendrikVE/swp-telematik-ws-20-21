#include "HTTPClient.h"
#include "Update.h"
#include "ArduinoLog.h"

#include "UpdateManager.h"
#include "../storage/FlashStorage.h"
#include "../MANIFEST.h"

#include "WiFi.h"
#include "WiFiClientSecure.h"

void UpdateManager::begin(const char* host, const char* filename) {
    this->mHost = host;
    this->mFilename = filename;
}

int UpdateManager::checkForOTAUpdate() {

    int rc = UPDATE_ERROR_OK;

    WiFiClientSecure client;
    client.setCACert( (char*) ca_crt_start  );
    client.setCertificate( (char*) client_crt_start  );
    client.setPrivateKey( (char*) client_key_start  );

    char request[256];
    //the APP_VERSION_CODE is from Manifest.h so that the ESP doesnt update itself forever, always looks
    //for code in folder one version higher 
    sprintf(request, "https://%s:4443/%s/%s/%s", this->mHost, CONFIG_DEVICE_ID, String(APP_VERSION_CODE+1).c_str(), this->mFilename);

    if(!mHttpClient.begin(client, request)) {
        Log.notice("Unable to connect");
        mHttpClient.end();
        return -UPDATE_ERROR_ABORT;
    }

    int httpCode = mHttpClient.GET();
    if (httpCode != HTTP_CODE_OK) {

        Log.notice("HTTP GET... failed in UpdateManager. error: %d", httpCode);

        if (httpCode == HTTP_CODE_NOT_FOUND) {
            Log.notice("Either there are no new updates or they can't be found.");
            rc = UPDATE_ERROR_OK;
        }
        else {
            rc = -UPDATE_ERROR_ABORT;
        }

        Log.notice("Exiting OTA Update");
        mHttpClient.end();

        return rc;
    }

    int contentLength = mHttpClient.getSize();
    Log.notice("Got %d bytes from server", contentLength);

    if (contentLength) {

        bool canBegin = Update.begin(contentLength);
        if (canBegin) {
            Log.notice("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
            size_t written = Update.writeStream(*mHttpClient.getStreamPtr());

            if (written == contentLength) {
                Log.notice("Written : %d successfully", written);
            }
            else {
                Log.notice("Written only : %d/%d. Retry?", written, contentLength);
            }

            if (Update.end()) {
                Log.notice("OTA done!");

                if (Update.isFinished()) {
                    Log.notice("Update successfully completed. Rebooting.");
                    ESP.restart();
                }
                else {
                    rc = -Update.getError();
                    Log.notice("Update not finished? Something went wrong!");
                }
            }
            else {
                rc = -Update.getError();
                Log.notice("Error Occurred. Error #: %d", rc);
            }
        }
        else {
            rc = -Update.getError();
            Log.notice("Not enough space to begin OTA");
        }
    }
    else {
        Log.notice("There was no content in the response");
    }

    mHttpClient.end();

    return rc;
}
