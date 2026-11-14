#include <Arduino.h>
#include <Preferences.h>

#include "main.h"

#define SV_IDLE                   0
#define SV_POWER_ON               1
#define SV_RAISE                  2
#define SV_RETURN                 3
#define SV_MOVE                   4
#define SV_STOP                   5

static int svState = SV_IDLE;
static unsigned long svStateTime = 0;
static unsigned long svStartTime = 0;

//servomove: move to moveDeg and power off (-1: raise)
static int moveDeg = -1;

static int restDeg = SERVO_REST_DEFAULT;
static int upDeg = SERVO_UP_DEFAULT;
static int waitMs = SERVO_MS_DEFAULT;

static Preferences prefs;

static void setSvState(int s) {
  svState = s;
  svStateTime = millis();
}

static unsigned long getSvStateTime(void) {
  return millis() - svStateTime;
}

static bool isValidAngle(int deg) {
  return SERVO_DEG_MIN <= deg && deg <= SERVO_DEG_MAX;
}

static bool isValidTime(int ms) {
  return SERVO_MS_MIN <= ms && ms <= SERVO_MS_MAX;
}

static uint32_t degToDuty(int deg) {
  uint32_t us = SERVO_US_0DEG + (uint32_t)(SERVO_US_180DEG - SERVO_US_0DEG) * deg / 180;

  return us * ((1UL << SERVO_PWM_BITS) - 1) / SERVO_PERIOD_US;
}

static void startPWM(void) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(GPIO_SERVO, SERVO_PWM_HZ, SERVO_PWM_BITS);
#else
  ledcSetup(0, SERVO_PWM_HZ, SERVO_PWM_BITS);
  ledcAttachPin(GPIO_SERVO, 0);
#endif
}

static void writePWM(int deg) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(GPIO_SERVO, degToDuty(deg));
#else
  ledcWrite(0, degToDuty(deg));
#endif
}

//Stop pulses at the next period, without cutting the current pulse.
//LEDC applies the new duty from the next period
static void stopPulse(void) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(GPIO_SERVO, 0);
#else
  ledcWrite(0, 0);
#endif
}

//Stop PWM, fix the signal LOW, and cut the servo power
//Call stopPulse() and wait SERVO_STOP_MS before this, or the last pulse may be cut
static void powerOff(void) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcDetach(GPIO_SERVO);
#else
  ledcDetachPin(GPIO_SERVO);
#endif
  pinMode(GPIO_SERVO, OUTPUT);
  digitalWrite(GPIO_SERVO, LOW);
  digitalWrite(GPIO_SERVO_PWR_EN, LOW);
}

//Place to check the power supply before moving (always OK for now)
static bool checkPower(void) {
  return true;
}

void initServo(void) {
  pinMode(GPIO_SERVO_PWR_EN, OUTPUT);
  digitalWrite(GPIO_SERVO_PWR_EN, LOW);
  pinMode(GPIO_SERVO, OUTPUT);
  digitalWrite(GPIO_SERVO, LOW);

  prefs.begin("servo", true);
  restDeg = prefs.getInt("rest", SERVO_REST_DEFAULT);
  upDeg = prefs.getInt("up", SERVO_UP_DEFAULT);
  waitMs = prefs.getInt("ms", SERVO_MS_DEFAULT);
  prefs.end();

  if (!isValidAngle(restDeg) || !isValidAngle(upDeg)) {
    restDeg = SERVO_REST_DEFAULT;
    upDeg = SERVO_UP_DEFAULT;
  }
  if (!isValidTime(waitMs)) {
    waitMs = SERVO_MS_DEFAULT;
  }

  setSvState(SV_IDLE);
}

bool startServo(void) {
  if (svState != SV_IDLE) {
    return false;
  }

  moveDeg = -1;
  digitalWrite(GPIO_SERVO_PWR_EN, HIGH);
  svStartTime = millis();
  setSvState(SV_POWER_ON);

  return true;
}

//Move to the angle, then power off at the angle (for adjustment)
bool moveServo(int deg) {
  if (svState != SV_IDLE || !isValidAngle(deg)) {
    return false;
  }

  moveDeg = deg;
  digitalWrite(GPIO_SERVO_PWR_EN, HIGH);
  svStartTime = millis();
  setSvState(SV_POWER_ON);

  return true;
}

bool isServoBusy(void) {
  return svState != SV_IDLE;
}

void loopServo(void) {
  if (svState == SV_IDLE) {
    return;
  }

  //Safety limit for the whole sequence
  if (millis() - svStartTime > SERVO_LIMIT_MS) {
    powerOff();
    Serial.printf("servo: timeout, power off\n");
    showLEDError();
    setSvState(SV_IDLE);
    return;
  }

  switch (svState) {
  case SV_POWER_ON:
    if (getSvStateTime() >= SERVO_PWR_ON_MS) {
      if (!checkPower()) {
        powerOff();
        Serial.printf("servo: power NG\n");
        showLEDError();
        setSvState(SV_IDLE);
        break;
      }
      startPWM();
      if (moveDeg >= 0) {
        writePWM(moveDeg);
        setSvState(SV_MOVE);
      } else {
        writePWM(upDeg);
        setSvState(SV_RAISE);
      }
    }
    break;
  case SV_RAISE:
    if (getSvStateTime() >= (unsigned long)waitMs) {
      writePWM(restDeg);
      setSvState(SV_RETURN);
    }
    break;
  case SV_RETURN:
  case SV_MOVE:
    if (getSvStateTime() >= (unsigned long)waitMs) {
      stopPulse();
      setSvState(SV_STOP);
    }
    break;
  case SV_STOP:
    if (getSvStateTime() >= SERVO_STOP_MS) {
      powerOff();
      setSvState(SV_IDLE);
    }
    break;
  }
}

bool setServoAngle(int rest, int up) {
  if (!isValidAngle(rest) || !isValidAngle(up)) {
    return false;
  }

  restDeg = rest;
  upDeg = up;

  prefs.begin("servo", false);
  prefs.putInt("rest", restDeg);
  prefs.putInt("up", upDeg);
  prefs.end();

  return true;
}

bool setServoTime(int ms) {
  if (!isValidTime(ms)) {
    return false;
  }

  waitMs = ms;

  prefs.begin("servo", false);
  prefs.putInt("ms", waitMs);
  prefs.end();

  return true;
}

int getServoRest(void) {
  return restDeg;
}

int getServoUp(void) {
  return upDeg;
}

int getServoTime(void) {
  return waitMs;
}
