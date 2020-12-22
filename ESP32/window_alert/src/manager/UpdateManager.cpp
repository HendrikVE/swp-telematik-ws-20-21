#include "HTTPClient.h"
#include "Update.h"
#include "ArduinoLog.h"

#include "UpdateManager.h"
#include "../storage/FlashStorage.h"
#include "../MANIFEST.h"

#include "WiFi.h"
#include "WiFiClientSecure.h"

void UpdateManager::begin(const char* host, const char* filename, const char* user, const char* password, const char* deviceID) {

    this->mHost = host;
    this->mFilename = filename;
    this->mUser = user;
    this->mPassword = password;
    this->mDeviceID = deviceID;
}

void UpdateManager::checkForOTAUpdate() {


/*  char request[256];
    //sprintf(request, "https://%s/%s/%s/%s", this->mHost, this->mDeviceID, String(APP_VERSION_CODE + 1).c_str(), this->mFilename);
    //sprintf(request, "https://%s/%s", this->mHost, this->mFilename);
    sprintf(request, "/%s", this->mFilename);
    //sprintf(request, "https://192.168.178.28:4443/window_alert.bin");
    Log.notice("Request is: %s", request);
    

    Log.notice("Host is: %s", this->mHost);
    mHttpClient.begin(this->mHost, 4443, request, (char*) ca_crt_start, (char*) client_crt_start, (char*) client_key_start);
    mHttpClient.setAuthorization(this->mUser, this->mPassword);
    //mHttpClient.setUserAgent("Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:84.0) Gecko/20100101 Firefox/84.0");

    int httpCode = mHttpClient.GET();

    if (httpCode != HTTP_CODE_OK) {
        Log.notice("HTTP GET... failed in ota, error: %d", httpCode);

        Log.notice("Exiting OTA Update");

        mHttpClient.end();

        return;
    }
*/

    Log.notice("before creation");
    WiFiClientSecure client;
    client.setCACert( (char*) ca_crt_start  );
    client.setCertificate( (char*) client_crt_start  );
    client.setPrivateKey( (char*) client_key_start  );
    char request[256];
    sprintf(request,"https://%s:4443/%s",this->mHost,this->mFilename);
    if(!mHttpClient.begin(client,request))
    {
        Log.notice("Unable to connect");
        mHttpClient.end();
        return;
    }
    mHttpClient.setAuthorization(this->mUser, this->mPassword);
    int httpCode = mHttpClient.GET();
    if (httpCode != HTTP_CODE_OK) 
    {
        Log.notice("HTTP GET... failed in ota, error: %d", httpCode);
        Log.notice("Exiting OTA Update");
        mHttpClient.end();
        return;
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
                    Log.notice("Update not finished? Something went wrong!");
                }
            }
            else {
                Log.notice("Error Occurred. Error #: %d", Update.getError());
            }
        }
        else {
            Log.notice("Not enough space to begin OTA");
        }
    }
    else {
        Log.notice("There was no content in the response");
    }

    mHttpClient.end();
}
