#include "HTTPClient.h"
#include "Update.h"
#include "ArduinoLog.h"

class UpdateManager {

public:

    void begin() {
        logger.begin(LOG_LEVEL_VERBOSE, &Serial);
        logger.setPrefix(printTag);
        logger.setSuffix(printNewline);
    }

    void checkForOTAUpdate() {

        char request[256];
        sprintf(request, "https://%s/%s/%s/%s", (char*) CONFIG_OTA_HOST, (char*) CONFIG_DEVICE_ID, String(APP_VERSION_CODE + 1).c_str(), (char*) CONFIG_OTA_FILENAME);

        mHttpClient.begin((char*) CONFIG_OTA_HOST, 4443, request, (char*) ca_crt_start, (char*) client_crt_start, (char*) client_key_start);
        mHttpClient.setAuthorization(CONFIG_OTA_SERVER_USERNAME, CONFIG_OTA_SERVER_PASSWORD);

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
