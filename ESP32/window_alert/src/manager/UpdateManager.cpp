#include "HTTPClient.h"
#include "Update.h"
#include "ArduinoLog.h"

#include "UpdateManager.h"
#include "../storage/FlashStorage.h"
#include "../MANIFEST.h"

void UpdateManager::begin(const char* host, const char* filename, const char* user, const char* password, const char* deviceID) {

    logger.begin(LOG_LEVEL_VERBOSE, &Serial);
    logger.setPrefix(printTag);
    logger.setSuffix(printNewline);

    this->mHost = host;
    this->mFilename = filename;
    this->mUser = user;
    this->mPassword = password;
    this->mDeviceID = deviceID;
}

void UpdateManager::checkForOTAUpdate() {

    char request[256];
    sprintf(request, "https://%s/%s/%s/%s", this->mHost, this->mDeviceID, String(APP_VERSION_CODE + 1).c_str(), this->mFilename);

    mHttpClient.begin(this->mHost, 4443, request, (char*) ca_crt_start, (char*) client_crt_start, (char*) client_key_start);
    mHttpClient.setAuthorization(this->mUser, this->mPassword);

    int httpCode = mHttpClient.GET();

    if (httpCode != HTTP_CODE_OK) {
        logger.notice("HTTP GET... failed, error: %d", httpCode);

        logger.notice("Exiting OTA Update");

        mHttpClient.end();

        return;
    }

    int contentLength = mHttpClient.getSize();
    logger.notice("Got %d bytes from server", contentLength);

    if (contentLength) {

        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
            logger.notice("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
            size_t written = Update.writeStream(*mHttpClient.getStreamPtr());

            if (written == contentLength) {
                logger.notice("Written : %d successfully", written);
            }
            else {
                logger.notice("Written only : %d/%d. Retry?", written, contentLength);
            }

            if (Update.end()) {
                logger.notice("OTA done!");

                if (Update.isFinished()) {
                    logger.notice("Update successfully completed. Rebooting.");
                    ESP.restart();
                }
                else {
                    logger.notice("Update not finished? Something went wrong!");
                }
            }
            else {
                logger.notice("Error Occurred. Error #: %d", Update.getError());
            }
        }
        else {
            logger.notice("Not enough space to begin OTA");
        }
    }
    else {
        logger.notice("There was no content in the response");
    }

    mHttpClient.end();
}
