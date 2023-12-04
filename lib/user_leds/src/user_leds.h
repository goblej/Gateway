#ifndef USER_LEDS_H
#define USER_LEDS_H

/*****************************************************
 *
 * User LED hardware mapping
 *
 * User LEDs 1-4 are mapped to the hardware as follows:
 *
 * User LED #1 = LED 8, Red   = 0x80
 *                      Green = 0x40
 * User LED #2 = LED 7, Red   = 0x20
 *                      Green = 0x10
 * User LED #3 = LED 6, Red   = 0x08
 *                      Green = 0x04
 * User LED #4 = LED 5, Red   = 0x02
 *                      Green = 0x01
 ******************************************************/

/* Four user LEDs */
typedef enum
{
	USER_LED_01 = 1, /* LED 8 on the PCB */
	USER_LED_02 = 2, /* LED 7 */
	USER_LED_03 = 3, /* LED 6 */
	USER_LED_04 = 4  /* LED 5 */
} userLedIds_e;

/*****************************************************
 * Define three states for the four bi-colour LEDs:
 * - off
 * - red
 * - green
 * Not including the both on state, as not distinct 
 * enough to be useful
******************************************************/
typedef enum
{
	USER_LED_OFF	= 0x00,
	USER_LED_GREEN	= 0x01,
	USER_LED_RED	= 0x02
} userLedState_e;

void initUserLeds ( void );
void setAllUserLeds ( userLedState_e ledState );
void setUserLed ( userLedIds_e userLedId, userLedState_e ledState );

#endif /* USER_LEDS_H_ */
