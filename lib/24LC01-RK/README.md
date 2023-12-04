# 24LC01-RK

*Particle driver for the Microchip 24AA01/24LC01B/24FC01 I2C Serial EEPROM*

Yes, it's really small in size (128 byte, 1Kbit) but it it's cheap (US$0.19) [in single quantities](https://www.digikey.com/product-detail/en/microchip-technology/24LC01BT-I-OT/24LC01BT-I-OTCT-ND/721417) and it's tiny in the SOT-23/SC-70 package. It's also low-power, 1 uA in standby, so it's great for board identification.


## Using the code

Typically you create a global variable for the EEPROM:

```
EEPROM_24LC01 eeprom;
```

The first parameter is the Wire interface to use, typically Wire (D0/D1). On the Electron you can also use Wire1 (C4/C5).

There is no address option, as the I2C address is not configurable on the 24xx01.

From setup you typically call:

```
eeprom.begin();
```

To read and write data you use the readData and writeData methods:

```
bool readData(size_t eepromAddr, uint8_t *data, size_t dataLen);
bool writeData(size_t eepromAddr, const uint8_t *data, size_t dataLen);
```

You can also use get and put like the [EEPROM API](https://docs.particle.io/reference/firmware/photon/#eeprom). This makes it easy to convert code between using the built-in EEPROM and I2C EEPROM as you can use the calls nearly identically.

