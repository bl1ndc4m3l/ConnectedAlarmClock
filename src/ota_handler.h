/**
 * =============================================================================
 * OTA Handler - Header
 * =============================================================================
 * Handles Over-The-Air firmware updates
 */

#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <ArduinoOTA.h>
#include "SSD1306.h"

// External display object
extern SSD1306 display;

// Function declarations
void setupOTA();

#endif // OTA_HANDLER_H
