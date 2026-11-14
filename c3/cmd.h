#ifndef C3_CMD_H__
#define C3_CMD_H__

#define VERSION             "0.1.0"

#define CMD_HELP            "help"
#define CMD_INIT            "init"
#define CMD_RAISE           "raise"
#define CMD_SERVO           "servo"
#define CMD_SERVO_TIME      "servotime"
#define CMD_SERVO_GET       "servoget"
#define CMD_SERVO_MOVE      "servomove"

void initCmd(void);
int getDeviceID(void);
void setDeviceID(int id);
void onReceive(String &str);

#endif /* C3_CMD_H__ */
