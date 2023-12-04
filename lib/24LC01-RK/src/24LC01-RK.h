#ifndef __24LC01RK_H
#define __24LC01RK_H

#include "Particle.h"

/**
 * @brief Particle library for Microchip 24AA01/24LC01B/24FC01 1Kbit I2C Serial EEPROM
 *
 * Yes, it's really small (128 byte, 1Kbit) but it it's cheap ($0.19) in single quantities at
 * https://www.digikey.com/product-detail/en/microchip-technology/24LC01BT-I-OT/24LC01BT-I-OTCT-ND/721417
 * and it's tiny in the SOT-23/SC-70 package. It's also low-power, 1 uA in standby, so it's great for
 * board identification.
 *
 * One caveat: It uses a lot of I2C addresses (0x50 - 0x57), but this range is typically used by things
 * like other FRAM and EEPROM chips, so if you already had one, you'd probably use that instead.
 *
 * You typically create one of these objects as a global variable.
 *
 * Github: https://github.com/rickkas7/24LC01-RK
 * License: MIT (free for use, including in commercial products)
 */
class EEPROM_24LC01 {
public:
	/**
	 * @brief Construct the object.  You typically create one of these objects as a global variable.
	 *
	 * The optional argument is typically Wire (pins D0 and D1). On the Electron it can also be Wire1 (pins C4 and C5).
	 */
	EEPROM_24LC01(TwoWire &wire = Wire);
	virtual ~EEPROM_24LC01();

	/**
	 * @brief Typically called during setup() to start the Wire interface.
	 */
	void begin();

	/**
	 * @brief Returns the size ot the memory in bytes
	 */
	inline size_t length() { return MEMORY_SIZE; }

	/**
	 * @brief Erase the EEPROM to 0 values
	 *
	 * This operation takes seconds to complete.
	 */
	bool erase();

	/**
	 * @brief Get a value from EEPROM
	 *
	 * @param eepromAddr The address to read from 0-127.
	 *
	 * @param t The value to write. Can be a simple type (int, char, bool, float) or a struct of simple types.
	 */
    template <typename T> T &get(size_t eepromAddr, T &t) {
        readData(eepromAddr, (uint8_t *)&t, sizeof(T));
        return t;
    }

	/**
	 * @brief Set a value in EEPROM
	 *
	 * @param eepromAddr The address to write to 0-1023.
	 *
	 * @param t The value to write. Can be a simple type (int, char, bool, float) or a struct of simple types.
	 *
	 * Even though the hardware has restrictions on writing across page boundaries (128 byte pages), this call
	 * will split a write up into smaller pieces as necessary. You can write the whole 1024 bytes in one call
	 * if you want, though it will take several seconds.
	 */
    template <typename T> const T &put(size_t eepromAddr, const T &t) {
        writeData(eepromAddr, (const uint8_t *)&t, sizeof(T));
        return t;
    }

    /**
     * @brief Low level read call
     *
	 * @param eepromAddr The address to read from 0-1023.
     *
     * @param data Point to the buffer to hold the data.
     *
     * @param dataLen The number of bytes to read.
     */
	bool readData(size_t eepromAddr, uint8_t *data, size_t dataLen);


    /**
     * @brief Low level write call
     *
	 * @param eepromAddr The address to read from 0-1023.
     *
     * @param data Point to the buffer with the data to write to EEPROM
     *
     * @param dataLen The number of bytes to write.
	 *
	 * Even though the hardware has restrictions on writing across page boundaries (128 byte pages), this call
	 * will split a write up into smaller pieces as necessary. You can write the whole 1024 bytes in one call
	 * if you want, though it will take several seconds.
     */
	bool writeData(size_t eepromAddr, const uint8_t *data, size_t dataLen);

	/**
	 * @brief I2C address base
	 *
	 * Does not include the R/W bit, so 0x00 - 0x7F.
	 *
	 * 0b1010xxx = 0x50 = 120
	 */
	static const uint8_t DEVICE_ADDR = 0x50;

	/**
	 * @brief The number of bits of page addressing, 7 for the 24xx01.
	 */
	static const size_t BLOCK_BITS = 7;

	/**
	 * @brief Page size in bytes, derived from BLOCK_BITS (128 for 24xx01)
	 */
	static const size_t BLOCK_SIZE = (1 << BLOCK_BITS);

	/**
	 * @brief Number of pages.
	 *
	 * The 24LC01 is really small (128 bytes) so it only has one page. Other models have more
	 * pages.
	 */
	static const size_t NUM_BLOCKS = 1;

	/**
	 * @brief Size of memory in bytes. 128 for 24xx01
	 */
	static const size_t MEMORY_SIZE = (BLOCK_SIZE * NUM_BLOCKS);

	/**
	 * @brief Maximum number of bytes that can be written by the page write call
	 *
	 * Assumption: BLOCK_SIZE (128) is a multiple of PAGE_SIZE (8).
	 *
	 * Assumption: PAGE_SIZE is not larger than 31. That's the maximum I2C write size, so
	 * if that assumption becomes untrue then an additional check will be necessary in the
	 * write loop to not exceed the I2C write size.
	 */
	static const size_t PAGE_SIZE = 8;

protected:
	TwoWire &wire;
};


#endif // __24LC01RK_H
