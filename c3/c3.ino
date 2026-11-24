#include <Arduino.h>
#include <esp_mac.h>

#include "ble.h"
#include "cmd.h"
#include "led.h"
#include "main.h"
#include "servo.h"

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
  for (int ch = 1; ch <= SERVO_CH_NUM; ch++) {
    Serial.printf("servo %d: rest %d, up %d, %d ms\n",
                  ch, getServoRest(ch), getServoUp(ch), getServoTime());
  }
}

void loop(void) {
  loopBLE();
  loopServo();
  loopLED();
  delay(1);
}
