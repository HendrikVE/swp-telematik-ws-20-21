#include "HTTPClient.h"
#include "Update.h"

class UpdateManager {

private:

    HTTPClient http;

public:

    void checkForOTAUpdate() {

        char request[256];
        sprintf(request, "https://%s/%s/%s/%s", (char*) CONFIG_OTA_HOST, (char*) CONFIG_DEVICE_ID, String(APP_VERSION_CODE + 1).c_str(), (char*) CONFIG_OTA_FILENAME);

        http.begin((char*) CONFIG_OTA_HOST, 4443, request, (char*) ca_crt_start, (char*) client_crt_start, (char*) client_key_start);
        http.setAuthorization(CONFIG_OTA_SERVER_USERNAME, CONFIG_OTA_SERVER_PASSWORD);

        int httpCode = http.GET();

        if (httpCode != HTTP_CODE_OK) {
            Serial.print("[HTTP] GET... failed, error: ");
            Serial.println(httpCode);

            Serial.println("Exiting OTA Update");

            http.end();

            return;
        }

        int contentLength = http.getSize();
        Serial.println("Got " + String(contentLength) + " bytes from server");

        if (contentLength) {

            bool canBegin = Update.begin(contentLength);

            if (canBegin) {
                Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
                size_t written = Update.writeStream(*http.getStreamPtr());

                if (written == contentLength) {
                    Serial.println("Written : " + String(written) + " successfully");
                }
                else {
                    Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
                }

                if (Update.end()) {
                    Serial.println("OTA done!");

                    if (Update.isFinished()) {
                        Serial.println("Update successfully completed. Rebooting.");
                        ESP.restart();
                    }
                    else {
                        Serial.println("Update not finished? Something went wrong!");
                    }
                }
                else {
                    Serial.println("Error Occurred. Error #: " + String(Update.getError()));
                }
            }
            else {
                Serial.println("Not enough space to begin OTA");
            }
        }
        else {
            Serial.println("There was no content in the response");
        }

        http.end();
    }

};