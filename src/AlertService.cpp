#include "AlertService.h"

bool AlertService::isValidLevel(uint8_t level) {
    if (level >= 0 && level <= 2)
    {
        return true;
    }
    else {
        return false;
    }
}