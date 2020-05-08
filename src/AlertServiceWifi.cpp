#include "AlertServiceWifi.h"

AlertServiceWifi::AlertServiceWifi(char *ssid, char *password, uint16_t alertServicePort)
    : ssid_(ssid)
    , password_(password)
    , port(alertServicePort) {

}

bool AlertServiceWifi::setAlertLevel(uint8_t alertLevel, bool notify) {
    if ( isValidLevel(alertLevel) ) {
        alertLevel_ = alertLevel;
        isServerNotification_ = notify;
        return true;
    }
    else {
        return false;
    }
}

uint8_t AlertServiceWifi::getAlertLevel() {
    return alertLevel_;
}

void AlertServiceWifi::start() {
    Serial.print("Connecting to Wifi gateway SSID '");
    Serial.print(ssid_);
    Serial.println("'");

    // Start WiFi device
    WiFi.begin(ssid_, password_);
    
    // Wait for WiFi connection to access point
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("...no connection to Wifi gateway...");
        delay(1000);
    }
    
    Serial.print("Connected to gateway IP: ");
    Serial.print(WiFi.gatewayIP());
    Serial.print(", Local IP: ");
    Serial.println(WiFi.localIP());
    
    // Create server socket and start listening on the configured TCP port
    alertServer_.begin(port);

    Serial.print("Listening for connections on port ");
    Serial.println(port);

    xTaskCreate(
        alertServiceTask, // Task function
        "AlertServiceTask", // Name of task
        4096, // Stack size of the task
        this, // Parameter of the task
        5, // Priority of the task
        NULL // Task handle to keep track of created task
    );
}

void AlertServiceWifi::alertServiceTask(void *pvParameters) {
    AlertServiceWifi *pAlertService = (AlertServiceWifi*) pvParameters;
    
    while (true) {
        
        WiFiClient client = pAlertService->alertServer_.available();
            
        if (client) {

            Serial.println("Client connected");
        
            while (client.connected()) {

                // If the notification flag is active, send an update to the client
                if (pAlertService->isServerNotification_) {
                    uint8_t level = pAlertService->alertLevel_;
                    client.write(level);
                    pAlertService->isServerNotification_ = false;

                    Serial.print("Sent 1 byte: ");
                    Serial.print(level);
                    Serial.println(" (server notification)");
                }

                // Check if data has been received from the client
                int numAvailable = client.available();

                // Process the received data
                if (numAvailable > 0) {
                    Serial.print("Received ");
                    Serial.print(numAvailable);
                    Serial.print(" bytes: ");

                    // Store the first received value
                    int rxValue = client.read();
                    Serial.print(rxValue);

                    // Dump the remaining values to serial output
                    for (int i = 0; i < numAvailable - 1; i++) {
                        Serial.print(' ');
                        Serial.print( client.read() );
                    }

                    Serial.println();
                        
                    switch (rxValue) {
                        case 0x00: // Set alert level to received value
                            pAlertService->alertLevel_ = 0;
                            break;

                        case 0x01: // Set alert level to received value
                            pAlertService->alertLevel_ = 1;
                            break;

                        case 0x02: // Set alert level to received value
                            pAlertService->alertLevel_ = 2;
                            break;

                        default:
                            
                            if (rxValue < 0) {
                                Serial.println("RX Error");
                            }
                            else {
                                // Respond with current alert level
                                uint8_t level = pAlertService->alertLevel_;
                                client.write(level);

                                Serial.print("Sent 1 byte: ");
                                Serial.print(level);
                                Serial.println(" (response)");
                            }
                            break;
                    }

                }

                delay(10);
            }
        
            client.stop();
            Serial.println("Client disconnected");
        } // if client

        delay(500);
    } // while true
}