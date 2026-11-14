#include <Arduino.h>

#include "ble.h"
#include "cmd.h"
#include "main.h"

static int device_id = -1;

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
  txBLE("servo rest up    : Set angles of rest and up (0-180).\n");
  txBLE("servotime ms     : Set wait time at up and rest (100-700).\n");
  txBLE("servoget         : Show the servo settings.\n");
  txBLE("servomove deg    : Move to the angle and power off (for adjustment).\n");
  txBLE("  ver." VERSION "\n");
}

void initCmd(void) {
  device_id = -1;
}

int getDeviceID(void) {
  return device_id;
}

void setDeviceID(int id) {
  device_id = id;
}

void onReceive(String &str) {
  const char *cmd = str.c_str();
  char name[16] = "";
  int a = 0, b = 0, n;
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
    n = sscanf(cmd, CMD_SERVO " %d %d %c", &a, &b, &extra);
    sendResult(name, n == 2 && !isServoBusy() && setServoAngle(a, b));
  } else if (strcmp(name, CMD_SERVO_TIME) == 0) {
    n = sscanf(cmd, CMD_SERVO_TIME " %d %c", &a, &extra);
    sendResult(name, n == 1 && !isServoBusy() && setServoTime(a));
  } else if (strcmp(name, CMD_SERVO_MOVE) == 0) {
    n = sscanf(cmd, CMD_SERVO_MOVE " %d %c", &a, &extra);
    sendResult(name, n == 1 && moveServo(a));
  } else if (strcmp(name, CMD_SERVO_GET) == 0) {
    char buf[CMD_LEN];

    snprintf(buf, sizeof(buf), "d:%d servoget %d %d %d\n",
             getDeviceID(), getServoRest(), getServoUp(), getServoTime());
    txBLE(buf);
    Serial.print(buf);
  } else {
    sendResult("unknown", false);
  }
}
