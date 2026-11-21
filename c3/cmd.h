#ifndef C3_CMD_H__
#define C3_CMD_H__

#define CMD_LEN                   64

//Command
#define CMD_HELP            "help"
#define CMD_INIT            "init"
#define CMD_RAISE           "raise"
#define CMD_SERVO           "servo"
#define CMD_SERVO_TIME      "servotime"
#define CMD_SERVO_GET       "servoget"
#define CMD_SERVO_MOVE      "servomove"
#define CMD_UNLOCK          "unlock"
#define CMD_LOCK            "lock"

//Unlock: servo, servotime and servomove need "unlock key" before them
#define UNLOCK_KEY                "target3"
#define UNLOCK_KEY_LEN            32
//Lock again automatically after this
#define UNLOCK_MS                 (5 * 60 * 1000)
//Ignore unlock for a while after a wrong key
#define UNLOCK_FAIL_WAIT_MS       3000

void initCmd(void);
void lockCmd(void);
int getDeviceID(void);
void setDeviceID(int id);
void onReceive(const char *cmd);

#endif /* C3_CMD_H__ */
