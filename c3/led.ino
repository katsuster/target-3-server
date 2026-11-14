#include <Arduino.h>

#include "ble.h"
#include "main.h"

static unsigned long errorEnd = 0;
static int lastR = -1, lastG = -1, lastB = -1;

static void writeLED(uint8_t r, uint8_t g, uint8_t b) {
  if (r == lastR && g == lastG && b == lastB) {
    return;
  }

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  rgbLedWrite(GPIO_LED, r, g, b);
#else
  neopixelWrite(GPIO_LED, r, g, b);
#endif
  lastR = r;
  lastG = g;
  lastB = b;
}

void initLED(void) {
  writeLED(0, 0, 0);
}

//Red for a while (servo was powered off by the timeout)
void showLEDError(void) {
  errorEnd = millis() + LED_ERROR_MS;
}

void loopLED(void) {
  if ((long)(errorEnd - millis()) > 0) {
    writeLED(LED_LEVEL, 0, 0);
  } else if (isServoBusy()) {
    writeLED(0, LED_LEVEL, 0);
  } else if (isBLEConnected()) {
    writeLED(0, 0, LED_LEVEL);
  } else if ((millis() / LED_BLINK_MS) % 2 == 0) {
    writeLED(0, 0, LED_LEVEL);
  } else {
    writeLED(0, 0, 0);
  }
}
