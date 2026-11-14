#include <Arduino.h>
#include <esp_mac.h>

#include "ble.h"
#include "cmd.h"
#include "main.h"

void setup(void) {
  //Turn off the servo before anything else
  initServo();

  Serial.begin(115200);
  initCmd();
  initLED();
  initBLE();

  Serial.printf("target-3-server ver." VERSION "\n");

  uint8_t macBT[6];
  esp_read_mac(macBT, ESP_MAC_BT);
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                macBT[0], macBT[1], macBT[2], macBT[3], macBT[4], macBT[5]);
  Serial.printf("servo: rest %d, up %d, %d ms\n",
                getServoRest(), getServoUp(), getServoTime());
}

void loop(void) {
  loopBLE();
  loopServo();
  loopLED();
  delay(1);
}
