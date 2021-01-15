#include "HTTPClient.h"
#include "Update.h"
#include "ArduinoLog.h"

#include "UpdateManager.h"
#include "../storage/FlashStorage.h"
#include "../MANIFEST.h"

#include "WiFi.h"
#include "WiFiClientSecure.h"

void UpdateManager::begin(const char* host, const char* filename, const char* user, const char* password, const char* deviceID) 
{

    this->mHost = host;
    this->mFilename = filename;
    this->mUser = user;
    this->mPassword = password;
    this->mDeviceID = deviceID;
}

int UpdateManager::checkForOTAUpdate() 
{

    WiFiClientSecure client;
    client.setCACert( (char*) ca_crt_start  );
    client.setCertificate( (char*) client_crt_start  );
    client.setPrivateKey( (char*) client_key_start  );
    char request[256];
    //the APP_VERSION_CODE is from Manifest.h so that the ESP doesnt update itself forever, always looks
    //for code in folder one version higher 
    sprintf(request,"https://%s:4443/%s/%s",this->mHost,String(APP_VERSION_CODE+1).c_str(),this->mFilename);
    if(!mHttpClient.begin(client,request))
    {
        Log.notice("Unable to connect");
        mHttpClient.end();
        return -69;
    }
    mHttpClient.setAuthorization(this->mUser, this->mPassword);
    int httpCode = mHttpClient.GET();
    if (httpCode != HTTP_CODE_OK) 
    {
        Log.notice("HTTP GET... failed in ota. error: %d", httpCode);
        if(httpCode == HTTP_CODE_NOT_FOUND)
        {
            Log.notice("Probably there are no new updates.");
        }
        Log.notice("Exiting OTA Update");
        mHttpClient.end();
        return httpCode;
    }



    int contentLength = mHttpClient.getSize();
    Log.notice("Got %d bytes from server", contentLength);

    int errorCode = UPDATE_ERROR_OK;
    if (contentLength) 
    {
        bool canBegin = Update.begin(contentLength);
        if (canBegin) 
        {
            Log.notice("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
            size_t written = Update.writeStream(*mHttpClient.getStreamPtr());

            if (written == contentLength) 
            {
                Log.notice("Written : %d successfully", written);
            }
            else 
            {
                Log.notice("Written only : %d/%d. Retry?", written, contentLength);
            }

            if (Update.end()) 
            {
                Log.notice("OTA done!");

                if (Update.isFinished()) 
                {
                    Log.notice("Update successfully completed. Rebooting.");
                    ESP.restart();
                }
                else 
                {
                    errorCode = Update.getError();
                    Log.notice("Update not finished? Something went wrong!");
                }
            }
            else 
            {
                errorCode = Update.getError();
                Log.notice("Error Occurred. Error #: %d", errorCode);
            }
        }
        else 
        {
            errorCode = Update.getError();
            Log.notice("Not enough space to begin OTA");
        }
    }
    else 
    {
        Log.notice("There was no content in the response");
    }
    mHttpClient.end();
    return errorCode;
}
