#include <Arduino.h>

#include "ble.h"
#include "cmd.h"
#include "main.h"
#include "servo.h"

static int device_id = -1;

static bool unlocked = false;
static unsigned long unlockTime = 0;
static unsigned long unlockFailTime = 0;
static bool unlockFailed = false;

static void sendResult(const char *cmd, bool ok) {
  char buf[CMD_LEN];

  snprintf(buf, sizeof(buf), "d:%d %s %s\n", getDeviceID(), cmd, ok ? "OK" : "NG");
  txBLE(buf);
  Serial.print(buf);
}

static void cmdHelp(void) {
  txBLE("help             : Show this message.\n");
  txBLE("init devid       : Set device ID.\n");
  txBLE("raise            : Raise the targets.\n");
  txBLE("servo ch rest up : Set angles of rest and up (ch: 1-4, 0-180).\n");
  txBLE("servotime ms     : Set wait time at up and rest (100-700).\n");
  txBLE("servoget ch      : Show the servo settings (ch: 1-4).\n");
  txBLE("servomove ch deg : Move to the angle and power off (for adjustment).\n");
  txBLE("unlock key       : Allow servo, servotime and servomove for a while.\n");
  txBLE("lock             : Disallow servo, servotime and servomove.\n");
  txBLE("  ver." VERSION "\n");
}

//Compare elapsed time, safe for millis() overflow (about 49.7 days)
static bool isUnlocked(void) {
  if (unlocked && millis() - unlockTime >= UNLOCK_MS) {
    unlocked = false;
  }

  return unlocked;
}

static bool unlockCmd(const char *key) {
  if (unlockFailed && millis() - unlockFailTime < UNLOCK_FAIL_WAIT_MS) {
    return false;
  }

  if (strcmp(key, UNLOCK_KEY) != 0) {
    unlockFailed = true;
    unlockFailTime = millis();
    return false;
  }

  unlocked = true;
  unlockTime = millis();
  unlockFailed = false;

  return true;
}

void initCmd(void) {
  device_id = -1;
  lockCmd();
}

void lockCmd(void) {
  unlocked = false;
}

int getDeviceID(void) {
  return device_id;
}

void setDeviceID(int id) {
  device_id = id;
}

void onReceive(const char *cmd) {
  char name[16] = "";
  int a = 0, b = 0, c = 0, n;
  char extra;

  n = sscanf(cmd, "%15s", name);
  if (n != 1) {
    return;
  }

  Serial.printf("cmd: '%s'\n", cmd);

  if (strcmp(name, CMD_HELP) == 0) {
    cmdHelp();
  } else if (strcmp(name, CMD_INIT) == 0) {
    n = sscanf(cmd, CMD_INIT " %d %c", &a, &extra);
    if (n == 1 && a >= 0) {
      setDeviceID(a);
      sendResult(name, true);
    } else {
      sendResult(name, false);
    }
  } else if (strcmp(name, CMD_RAISE) == 0) {
    sendResult(name, startServo());
  } else if (strcmp(name, CMD_SERVO) == 0) {
    n = sscanf(cmd, CMD_SERVO " %d %d %d %c", &a, &b, &c, &extra);
    sendResult(name, n == 3 && isUnlocked() && !isServoBusy() && setServoAngle(a, b, c));
  } else if (strcmp(name, CMD_SERVO_TIME) == 0) {
    n = sscanf(cmd, CMD_SERVO_TIME " %d %c", &a, &extra);
    sendResult(name, n == 1 && isUnlocked() && !isServoBusy() && setServoTime(a));
  } else if (strcmp(name, CMD_SERVO_MOVE) == 0) {
    n = sscanf(cmd, CMD_SERVO_MOVE " %d %d %c", &a, &b, &extra);
    sendResult(name, n == 2 && isUnlocked() && moveServo(a, b));
  } else if (strcmp(name, CMD_UNLOCK) == 0) {
    char key[UNLOCK_KEY_LEN];

    n = sscanf(cmd, CMD_UNLOCK " %31s %c", key, &extra);
    sendResult(name, n == 1 && unlockCmd(key));
  } else if (strcmp(name, CMD_LOCK) == 0) {
    lockCmd();
    sendResult(name, true);
  } else if (strcmp(name, CMD_SERVO_GET) == 0) {
    char buf[CMD_LEN];

    n = sscanf(cmd, CMD_SERVO_GET " %d %c", &a, &extra);
    if (n != 1 || getServoRest(a) < 0) {
      sendResult(name, false);
      return;
    }
    snprintf(buf, sizeof(buf), "d:%d servoget %d %d %d %d\n",
             getDeviceID(), a, getServoRest(a), getServoUp(a), getServoTime());
    txBLE(buf);
    Serial.print(buf);
  } else {
    sendResult("unknown", false);
  }
}
