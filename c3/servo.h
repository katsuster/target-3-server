#ifndef C3_SERVO_H__
#define C3_SERVO_H__

//GPIO
#define GPIO_PWR_EN               4
#define GPIO_M1_PWM1              5
#define GPIO_M1_PWM2              6
#define GPIO_M2_PWM1              7
#define GPIO_M2_PWM2              10

//Channel 1-4: M1_PWM1, M1_PWM2, M2_PWM1, M2_PWM2
#define SERVO_CH_NUM              4

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
//Start the 2nd servo of a motor board later than the 1st one,
//not to overlap the inrush currents on the same USB power
#define SERVO_STAGGER_MS          50
//Wait after the last pulse, before PWM stops (longer than a period)
#define SERVO_STOP_MS             25
//Power off forcibly if the whole sequence exceeds this
//(longest: PWR_ON 10 + STAGGER 50 + MS_MAX 700 * 2 + STOP 25 = 1485ms)
#define SERVO_LIMIT_MS            1600

//Servo, ch: 1 to SERVO_CH_NUM
void initServo(void);
void loopServo(void);
bool startServo(void);
bool isServoBusy(void);
bool moveServo(int ch, int deg);
bool setServoAngle(int ch, int rest, int up);
bool setServoTime(int ms);
int getServoRest(int ch);
int getServoUp(int ch);
int getServoTime(void);

#endif /* C3_SERVO_H__ */
