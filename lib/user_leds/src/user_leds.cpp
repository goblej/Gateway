#include "Particle.h"
#include "MCP23008-RK.h"
#include "user_leds.h"

uint8_t userLedBits;

MCP23008 mcp23008 ( Wire, 0 );

/*****************************************************
 * Used for power up lamp test, when all of the LEDs
 * are illumminated simultaneously
******************************************************/
void initUserLeds ( void )
{
  /* Four bi-colour user LEDs are implemented with an MCP23008
   * Note that the MCP23008 library has been modified slightly to 
   * provide byte access, which is better suited to our purposes 
   * Set outputs low to turn LED on, high to turn LED off */
  mcp23008.begin();

  /* Set LEDs all off */
  setAllUserLeds ( USER_LED_OFF );

  /* Then set all GPIO as outputs */
  mcp23008.writeRegister(0, 0x00);
}
  
/*****************************************************
 * Used for power up lamp test, when all of the LEDs
 * are illuminated simultaneously
******************************************************/
void setAllUserLeds ( userLedState_e ledState )
{
	switch ( ledState )
	{
		case USER_LED_GREEN:	userLedBits = 0x55; break;
		case USER_LED_RED: 		userLedBits = 0xaa; break;
		case USER_LED_OFF: 		userLedBits = 0x00; break;
	}
	
	/* Write updated value to the GPIO expander */
	mcp23008.writeRegister(9, ~userLedBits);
}

void setUserLed ( userLedIds_e userLedId, userLedState_e ledState )
{
	/* Clear previous LED state (red and green both off) */
	userLedBits &= ~( 0x03 << ( 2 * ( 4 - userLedId ) ) );
	
	/* Or-in new LED state */
	userLedBits |= ( ledState << ( 2 * ( 4 - userLedId ) ) );
	
	/* Write updated value to the GPIO expander */
	mcp23008.writeRegister(9, ~userLedBits);
}
