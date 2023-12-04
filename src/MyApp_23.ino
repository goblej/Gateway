/*
 * Project MyApp_23
 * Description:
 * Author:
 * Date:
 */

#include "build.h"
#include "eeprom.h"
#include "cli.h"
#include "at_commands.h"
#include "panel_protocol.h"
#include "MCP23008-RK.h"
#include "24LC01-RK.h"
#include "user_leds.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

EEPROM_24LC01 baseboardEeprom;

/* These are the 'magic number' bytes so that we can determine 
 * if the configuration has been initialized or not
 * A checksum might be a good idea too */
const uint32_t baseboardEepromDataMagic = 0x8684f2cd;
const uint32_t particleEepromDataMagic = 0x797b0d35;

// Offset in the Baseboard EEPROM where our identification data is held
const int baseboardEepromDataOffset = 0;

// Offset in the Particle EEPROM where our config data is held
const int particleEepromDataOffset = 0;

// A copy of the config data is held in RAM. This allows for more
// efficient updates and reduced EEPROM access
// Possibly. Not sure about this, since access is very rare anyway
// Keep for now
baseboardEepromData_t baseboardEepromData;
particleEepromData_t particleEepromData;

/* Pointer to a function used to process incoming serial data */
/* The pointer is assigned to the approriate protocol handler */
/* during protocol initialisation                             */
extern protocolRxHandler_t protocolRxHandler;

SerialLogHandler logHandler;

unsigned long lastTime = 0;

/* A state machine is used to implement a short power up test sequence */
uint8_t gatewayState = 0;

/* Product Identification */
/* Used in conjunction with supplementary configurable identification
 * information held in EEPROM */
char	hostModuleType [ 32 ] = "Particle B524";
char	gatewayFirmwareVer [ 8 ] = "01";
char	protocolLibraryVer [ 8 ] = "01";
uint8_t		atCommandAccessLevel = 0;

// Used to power up the isolated serial interface
int pwrEnablePin = D23;

uint8_t stringBuf[256];
size_t stringLen;
uint8_t gpioBits, lutIndex, toggle;
unsigned long nextTime = 0;
float tmp = 96.0;
BleAdvertisingData data;
CellularSignal signal;
const uint8_t CellularLevelBargraphLED [] = { 0xff, 0xfd, 0xf5, 0xd5, 0x55, 0xfe, 0xfa, 0xea, 0xaa };

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

/*****************************************************************************
 * Setup for BLE UART Peripheral
 * The UUIDs below are proprietary. However they're the ones used by Nordic 
 * Semiconductor for their UART example, and have been widely adopted for 
 * UART-like services over Bluetooth. For example the popular Adafruit 
 * Bluefruit hardware and mobile apps support this approach
 *****************************************************************************/
const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
  uint8_t rx_char;
    for (size_t ii = 0; ii < len; ii++) {
        rx_char = data[ii];

        if (rx_char == CR) {
            edit_accept_line();
        }
        else {
            /* normal character processing */
            add_char_to_line_buffer(rx_char);
        }
    }
}

void printProductIdentification()
{
	// Hardware identification information
	Serial.printf( "%s\n", baseboardEepromData.manufacturer);
	Serial.printf( "%s, %s\n", baseboardEepromData.baseboardType, baseboardEepromData.baseboardRevision);
	Serial.printf( "SN: %s\n", baseboardEepromData.baseboardSerialNo);

  // Module type and software revisions
	Serial.printf( "%s, %s\n", hostModuleType, System.deviceID().c_str());
	Serial.printf( "Device OS: %s\n", System.version().c_str());
	Serial.printf( "Gateway firmware:  %s\n", gatewayFirmwareVer);
	Serial.printf( "Protocol library:  %s\n", protocolLibraryVer);
}

void bluetoothWriteStr ( char * str );

void bluetoothWriteStr ( char * str )
{
    size_t len = strlen ( str );
    if (BLE.connected()) {
      txCharacteristic.setValue ( (uint8_t *) str, len );
    }
}

void resetBaseboardEeprom ( void )
{
  memset(&baseboardEepromData, 0, sizeof(baseboardEepromData));

  baseboardEepromData.magic = baseboardEepromDataMagic;
  strcpy(baseboardEepromData.manufacturer, "Nimbus Digital Solutions Ltd");
  strcpy(baseboardEepromData.baseboardType, "Nimbus Mobile Gateway Gen 3");
  strcpy(baseboardEepromData.baseboardRevision, "Rev A");
  strcpy(baseboardEepromData.baseboardSerialNo, "0000000"); // ??? Probably not a good idea, 
  baseboardEeprom.put(baseboardEepromDataOffset, baseboardEepromData);
}

void resetParticleEeprom ( void )
{
  memset(&particleEepromData, 0, sizeof(particleEepromData));

  particleEepromData.magic = particleEepromDataMagic;
  particleEepromData.protocolId = 0; // None
  particleEepromData.panelSerialBaud = 38400UL;
  particleEepromData.nimbusSessionId = 0UL;
  particleEepromData.serialFramingId = 0;
  particleEepromData.timestampFormat = 0;
  particleEepromData.cellPower = 0;
  particleEepromData.morleyZxPanelAddr = 1;
  particleEepromData.enableNimbusTransfers = 0;
  strcpy(particleEepromData.nimbusTargetServer, "nimbus/dev");
  strcpy(particleEepromData.atCommandPassword, "letmein");
  EEPROM.put(particleEepromDataOffset, particleEepromData);
}

// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.

  /* Open serial console over USB */
  /* Not essential for Gateway operation, but useful for logging messages */
  Serial.begin();

  /* Wait for host to establish serial connection */
  waitFor(Serial.isConnected, 5000);

  /* Set this flag so that we can determine the reason for any reset */
  System.enableFeature(FEATURE_RESET_INFO);

  /* Startup message */
  Logger log("app.main");

  // Read product identification and config data from the EEPROM
  EEPROM.get(particleEepromDataOffset, particleEepromData);
  if (particleEepromData.magic != particleEepromDataMagic) {
      /* First use or corrupt, just initialise everything */
      log.info("Particle EEPROM first use or corrupt, restoring default configuration");
      resetParticleEeprom ();
  }

  // Read product identification data from the Baseboard EEPROM
  baseboardEeprom.get(baseboardEepromDataOffset, baseboardEepromData);
  if (baseboardEepromData.magic != baseboardEepromDataMagic) {
      /* First use or corrupt, just initialise everything */
      log.info("Baseboard EEPROM first use or corrupt, restoring default configuration");
      resetBaseboardEeprom ();
  }

  /* First console message is always product identification */
  printProductIdentification();

  /* Initialise User LEDs on GPIO expander */
  initUserLeds ();

  /* Bluetooth */
  BLE.on();
  BLE.addCharacteristic(txCharacteristic);
  BLE.addCharacteristic(rxCharacteristic);
  data.appendServiceUUID(serviceUuid);
  BLE.advertise(&data);

  /* Isolated Power
   * Control power to the isolated serial interface
   * Set to low/high to turn isolated power off/on respectively */
  pinMode(pwrEnablePin, OUTPUT);

  /* Turn on isolated power */
  digitalWrite(pwrEnablePin, HIGH);

  /* Cellular Power
   * Keep it off for now, while I'm doing testing */
  if ( particleEepromData.cellPower == 1 ) {
    Cellular.on();
    Particle.connect();
  }

  /* Start the configured serial protocol */
  setProtocolType ( particleEepromData.protocolId );

	/* Initialise the AT command handler */
	at_Initialise();
}

int powerSource;
void updateChargeStatusLed ( void )
{
  switch ( System.batteryState() )
  {
    case BATTERY_STATE_CHARGING:  setUserLed(USER_LED_02, USER_LED_RED); break;
    case BATTERY_STATE_CHARGED:   setUserLed(USER_LED_02, USER_LED_GREEN); break;
    default: setUserLed(USER_LED_02, USER_LED_OFF); break;
  }
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.

  // Use a state machine to run through a short power-up test sequence
  // before falling into main application
  // The power-up test exercises the buzzer and User LEDs (red then green)
  switch ( gatewayState)
  {
    case 0:
      if (millis() >= nextTime){
        nextTime = millis() + 1000;

        // Sound Buzzer for 500ms
        // Buzzer has a nominal frequency of 3KHz, so use that
        tone(D6, 3000, 500);

        // And start User LED Test (green) for 1s
        setAllUserLeds ( USER_LED_GREEN );

        // Next state
        gatewayState = 1;
      }
      break;
    case 1:
      if (millis() >= nextTime){
        nextTime = millis() + 1000;

        // Cancel User LED Test (green) and start User LED Test (red)
        setAllUserLeds ( USER_LED_RED );

        // Next state
        gatewayState = 2;
      }
      break;
    case 2:
      if (millis() >= nextTime){
        nextTime = millis();

        // User LED Test (all User LEDs off)
        setAllUserLeds ( USER_LED_OFF );

        // Next state
        gatewayState = 3;
      }
      break;

    case 3:
      // Power-up tests complete
      // Commence normal operation

      /* Parse any incoming AT commands */
      cliScan();

      /* Process any incoming characters from the configured serial protocol */
      serialScan();

      /* Housekeeping */
      /* Do this less often */
      if (millis() >= nextTime){
        nextTime = millis() + 1000;

        updateChargeStatusLed();
      }
      break;
  }
}