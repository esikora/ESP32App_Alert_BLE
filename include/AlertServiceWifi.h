#include "AlertService.h"

// Library: ESP32 WiFi Arduino
#include <WiFi.h>

class AlertServiceWifi : public AlertService {

    public:

        AlertServiceWifi(char *ssid, char *password, uint16_t alertServicePort);

        // Sets the value of the alert level characteristic
        bool setAlertLevel(uint8_t alertLevel, bool notify);

        // Returns the current value of the alert level characteristic
        uint8_t getAlertLevel();

        // Initializes and starts everything to provide the immediate alert service
        void start();

    private:

        char *ssid_     = nullptr;
        char *password_ = nullptr;
        uint16_t port   = 0;

        WiFiServer alertServer_;

        uint8_t alertLevel_ = 0;
        bool isServerNotification_ = false;

        static void alertServiceTask(void *pvParameters);

};