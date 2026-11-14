#ifndef C3_MAIN_H__
#define C3_MAIN_H__

#include <Arduino.h>

//GPIO
#define GPIO_LED                  2
#define GPIO_SERVO                6
#define GPIO_SERVO_PWR_EN         19

//Command
#define CMD_LEN                   64

//Servo
#define SERVO_PWM_HZ              50
#define SERVO_PWM_BITS            14
#define SERVO_PERIOD_US           20000
//Pulse width for 0 and 180 degrees (measure on the actual servo)
#define SERVO_US_0DEG             500
#define SERVO_US_180DEG           2500
#define SERVO_DEG_MIN             0
#define SERVO_DEG_MAX             180

#define SERVO_REST_DEFAULT        60
#define SERVO_UP_DEFAULT          120
#define SERVO_MS_DEFAULT          400
#define SERVO_MS_MIN              100
#define SERVO_MS_MAX              700

//Wait after SERVO_PWR_EN is High, before PWM starts
#define SERVO_PWR_ON_MS           10
//Wait after the last pulse, before PWM stops (longer than a period)
#define SERVO_STOP_MS             25
//Power off forcibly if the whole sequence exceeds this
#define SERVO_LIMIT_MS            1500

//LED
#define LED_LEVEL                 64
#define LED_BLINK_MS              250
#define LED_ERROR_MS              3000

//Servo
void initServo(void);
void loopServo(void);
bool startServo(void);
bool isServoBusy(void);
bool moveServo(int deg);
bool setServoAngle(int rest, int up);
bool setServoTime(int ms);
int getServoRest(void);
int getServoUp(void);
int getServoTime(void);

//LED
void initLED(void);
void showLEDError(void);
void loopLED(void);

#endif /* C3_MAIN_H__ */
