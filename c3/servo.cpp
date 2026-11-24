#include <Arduino.h>
#include <Preferences.h>

#include "led.h"
#include "servo.h"

#define SV_IDLE                   0
#define SV_POWER_ON               1
#define SV_RUN                    2
#define SV_STOP                   3

//State of each channel in SV_RUN
#define CH_SKIP                   0
#define CH_WAIT                   1
#define CH_RAISE                  2
#define CH_RETURN                 3
#define CH_MOVE                   4
#define CH_DONE                   5

static const int chPin[SERVO_CH_NUM] = {
  GPIO_M1_PWM1, GPIO_M1_PWM2, GPIO_M2_PWM1, GPIO_M2_PWM2,
};
//The 2nd servo of each motor board starts later
static const unsigned long chStagger[SERVO_CH_NUM] = {
  0, SERVO_STAGGER_MS, 0, SERVO_STAGGER_MS,
};

static int svState = SV_IDLE;
static unsigned long svStateTime = 0;
static unsigned long svStartTime = 0;

static int chState[SERVO_CH_NUM];
static unsigned long chStateTime[SERVO_CH_NUM];
static unsigned long chDelay[SERVO_CH_NUM];
static bool chAttached[SERVO_CH_NUM];

//servomove: move to moveDeg and power off (-1: raise)
static int moveDeg = -1;

static int restDeg[SERVO_CH_NUM];
static int upDeg[SERVO_CH_NUM];
static int waitMs = SERVO_MS_DEFAULT;

static Preferences prefs;

static void setSvState(int s) {
  svState = s;
  svStateTime = millis();
}

static unsigned long getSvStateTime(void) {
  return millis() - svStateTime;
}

static void setChState(int i, int s) {
  chState[i] = s;
  chStateTime[i] = millis();
}

static unsigned long getChStateTime(int i) {
  return millis() - chStateTime[i];
}

static bool isValidCh(int ch) {
  return 1 <= ch && ch <= SERVO_CH_NUM;
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

static void startPWM(int i) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(chPin[i], SERVO_PWM_HZ, SERVO_PWM_BITS);
#else
  ledcSetup(i, SERVO_PWM_HZ, SERVO_PWM_BITS);
  ledcAttachPin(chPin[i], i);
#endif
  chAttached[i] = true;
}

static void writePWM(int i, int deg) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(chPin[i], degToDuty(deg));
#else
  ledcWrite(i, degToDuty(deg));
#endif
}

//Stop pulses at the next period, without cutting the current pulse.
//LEDC applies the new duty from the next period
static void stopPulse(int i) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(chPin[i], 0);
#else
  ledcWrite(i, 0);
#endif
}

//Stop PWM, fix the signals LOW, and cut the servo power
//Call stopPulse() and wait SERVO_STOP_MS before this, or the last pulse may be cut
static void powerOff(void) {
  for (int i = 0; i < SERVO_CH_NUM; i++) {
    if (chAttached[i]) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
      ledcDetach(chPin[i]);
#else
      ledcDetachPin(chPin[i]);
#endif
      chAttached[i] = false;
    }
    pinMode(chPin[i], OUTPUT);
    digitalWrite(chPin[i], LOW);
  }
  digitalWrite(GPIO_PWR_EN, LOW);
}

//Place to check the power supply before moving (always OK for now)
static bool checkPower(void) {
  return true;
}

static void loadPrefs(void) {
  char key[8];

  prefs.begin("servo", true);
  for (int i = 0; i < SERVO_CH_NUM; i++) {
    snprintf(key, sizeof(key), "rest%d", i + 1);
    restDeg[i] = prefs.getInt(key, SERVO_REST_DEFAULT);
    snprintf(key, sizeof(key), "up%d", i + 1);
    upDeg[i] = prefs.getInt(key, SERVO_UP_DEFAULT);

    if (!isValidAngle(restDeg[i]) || !isValidAngle(upDeg[i])) {
      restDeg[i] = SERVO_REST_DEFAULT;
      upDeg[i] = SERVO_UP_DEFAULT;
    }
  }
  waitMs = prefs.getInt("ms", SERVO_MS_DEFAULT);
  prefs.end();

  if (!isValidTime(waitMs)) {
    waitMs = SERVO_MS_DEFAULT;
  }
}

void initServo(void) {
  pinMode(GPIO_PWR_EN, OUTPUT);
  digitalWrite(GPIO_PWR_EN, LOW);
  for (int i = 0; i < SERVO_CH_NUM; i++) {
    pinMode(chPin[i], OUTPUT);
    digitalWrite(chPin[i], LOW);
    chAttached[i] = false;
    chState[i] = CH_SKIP;
  }

  loadPrefs();

  setSvState(SV_IDLE);
}

static void powerOn(void) {
  digitalWrite(GPIO_PWR_EN, HIGH);
  svStartTime = millis();
  setSvState(SV_POWER_ON);
}

//Raise all channels
bool startServo(void) {
  if (svState != SV_IDLE) {
    return false;
  }

  moveDeg = -1;
  for (int i = 0; i < SERVO_CH_NUM; i++) {
    chState[i] = CH_WAIT;
    chDelay[i] = chStagger[i];
  }
  powerOn();

  return true;
}

//Move a channel to the angle, then power off at the angle (for adjustment)
bool moveServo(int ch, int deg) {
  if (svState != SV_IDLE || !isValidCh(ch) || !isValidAngle(deg)) {
    return false;
  }

  moveDeg = deg;
  for (int i = 0; i < SERVO_CH_NUM; i++) {
    chState[i] = (i == ch - 1) ? CH_WAIT : CH_SKIP;
    chDelay[i] = 0;
  }
  powerOn();

  return true;
}

bool isServoBusy(void) {
  return svState != SV_IDLE;
}

//Returns true if the channel finished
static bool loopCh(int i) {
  switch (chState[i]) {
  case CH_WAIT:
    if (getSvStateTime() >= chDelay[i]) {
      startPWM(i);
      if (moveDeg >= 0) {
        writePWM(i, moveDeg);
        setChState(i, CH_MOVE);
      } else {
        writePWM(i, upDeg[i]);
        setChState(i, CH_RAISE);
      }
    }
    break;
  case CH_RAISE:
    if (getChStateTime(i) >= (unsigned long)waitMs) {
      writePWM(i, restDeg[i]);
      setChState(i, CH_RETURN);
    }
    break;
  case CH_RETURN:
  case CH_MOVE:
    if (getChStateTime(i) >= (unsigned long)waitMs) {
      stopPulse(i);
      setChState(i, CH_DONE);
    }
    break;
  }

  return chState[i] == CH_SKIP || chState[i] == CH_DONE;
}

void loopServo(void) {
  bool done = true;

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
      setSvState(SV_RUN);
    }
    break;
  case SV_RUN:
    for (int i = 0; i < SERVO_CH_NUM; i++) {
      if (!loopCh(i)) {
        done = false;
      }
    }
    if (done) {
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

bool setServoAngle(int ch, int rest, int up) {
  char key[8];

  if (!isValidCh(ch) || !isValidAngle(rest) || !isValidAngle(up)) {
    return false;
  }

  restDeg[ch - 1] = rest;
  upDeg[ch - 1] = up;

  prefs.begin("servo", false);
  snprintf(key, sizeof(key), "rest%d", ch);
  prefs.putInt(key, rest);
  snprintf(key, sizeof(key), "up%d", ch);
  prefs.putInt(key, up);
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

//Returns -1 if ch is invalid
int getServoRest(int ch) {
  return isValidCh(ch) ? restDeg[ch - 1] : -1;
}

//Returns -1 if ch is invalid
int getServoUp(int ch) {
  return isValidCh(ch) ? upDeg[ch - 1] : -1;
}

int getServoTime(void) {
  return waitMs;
}
