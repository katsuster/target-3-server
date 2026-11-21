#ifndef C3_LED_H__
#define C3_LED_H__

//GPIO
#define GPIO_LED                  2

//LED
#define LED_LEVEL                 64
#define LED_BLINK_MS              250
#define LED_ERROR_MS              3000

//LED
void initLED(void);
void showLEDError(void);
void loopLED(void);

#endif /* C3_LED_H__ */
