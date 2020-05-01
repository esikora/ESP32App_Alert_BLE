#include <Arduino.h>

/**
 * Abstract base class for the Alert Service.
 */
class AlertService {

    public:

        // Sets the alert level to the specified value
        virtual bool setAlertLevel(uint8_t alertLevel, bool notify) = 0;

        // Returns the current alert level
        virtual uint8_t getAlertLevel() = 0;

        // Initializes and starts the service
        virtual void start() = 0;

        bool isValidLevel(uint8_t level);
};