
#include "24LC01-RK.h"

static Logger log("app.eeprom");

#define ENABLE_TRACE 0
#if ENABLE_TRACE
#define TRACE_DEBUG(x) log.trace x
#else
#define TRACE_DEBUG(x)
#endif

EEPROM_24LC01::EEPROM_24LC01(TwoWire &wire) :
	wire(wire) {
}

EEPROM_24LC01::~EEPROM_24LC01() {
}

void EEPROM_24LC01::begin() {
	wire.begin();
}

bool EEPROM_24LC01::erase() {

	size_t eepromAddr = 0;
	size_t totalLen = MEMORY_SIZE;

	uint8_t zero[PAGE_SIZE];
	memset(zero, 0, sizeof(zero));

	while(totalLen > 0) {
		// Assumption: MEMORY_SIZE (128) is a multiple of PAGE_SIZE (8).
		bool result = writeData(eepromAddr, zero, sizeof(zero));
		if (!result) {
			return false;
		}

		totalLen -= sizeof(zero);
		eepromAddr += sizeof(zero);
	}
	return true;
}


bool EEPROM_24LC01::readData(size_t eepromAddr, uint8_t *data, size_t dataLen) {
	bool result = true;

	eepromAddr &= (MEMORY_SIZE - 1);

	TRACE_DEBUG(("readData eepromAddr=%u block=%u addr=%u", eepromAddr, eepromAddr >> BLOCK_BITS, eepromAddr & (BLOCK_SIZE - 1)));

	uint8_t i2cAddr = (uint8_t)((eepromAddr >> BLOCK_BITS) | DEVICE_ADDR);

	for(size_t tries = 1; tries <= 5; tries++) {
		wire.beginTransmission(i2cAddr);
		wire.write(eepromAddr & (BLOCK_SIZE - 1));
		int stat = wire.endTransmission(false);
		if (stat == 0) {
			result = true;
			break;
		}
		// It's quite likely this will fail once or twice periodically. If the EEPROM is still
		// busy, this will be the case so we just retry a few times.
		TRACE_DEBUG(("readData set page and addr failed stat=%d tries=%u", stat, tries));
		delay(100);
		result = false;
	}

	if (!result) {
		log.info("readData set page and addr failed");
		return false;
	}

	while(dataLen > 0) {
		size_t bytesToRead = dataLen;
		if (bytesToRead > 32) {
			bytesToRead = 32;
		}

		TRACE_DEBUG(("reading %u bytes", bytesToRead));

		wire.requestFrom((uint8_t)((eepromAddr >> BLOCK_BITS) | DEVICE_ADDR), (uint8_t) bytesToRead, (uint8_t) true);

		if (Wire.available() < (int) bytesToRead) {
			log.info("read failed, did not get enough bytes");
			result = false;
			break;
		}

		for(size_t ii = 0; ii < bytesToRead; ii++) {
		    *data++ = Wire.read();    // receive a byte as character
		    eepromAddr++;
		    dataLen--;
		}
	}

	return result;
}


bool EEPROM_24LC01::writeData(size_t eepromAddr, const uint8_t *data, size_t dataLen) {
	bool result = true;

	eepromAddr &= (MEMORY_SIZE - 1);

	TRACE_DEBUG(("writeData eepromAddr=%u dataLen=%u", eepromAddr, dataLen));

	while(dataLen > 0) {
		uint8_t i2cAddr = (uint8_t)((eepromAddr >> BLOCK_BITS) | DEVICE_ADDR);

		for(size_t writeTries = 1; writeTries <= 5; writeTries++) {
			wire.beginTransmission(i2cAddr);

			size_t blockNum = eepromAddr & (BLOCK_SIZE - 1);
			wire.write(blockNum);

			// Assumption: BLOCK_SIZE is a multiple of PAGE_SIZE, so if we break down writes into
			// pages (required by the hardware), it will always also break down on block boundaries.
			size_t bytesToWrite = dataLen;

			size_t pageOffset = eepromAddr & (PAGE_SIZE - 1);
			if (bytesToWrite > (PAGE_SIZE - pageOffset)) {
				// Writes cannot span a page boundary (8 bytes), so adjust to not cross and do a
				// second write later.
				bytesToWrite = (PAGE_SIZE - pageOffset);
			}

			TRACE_DEBUG(("writeData eepromAddr=%u page=%u blockNum=%u bytesToWrite=%u", eepromAddr, eepromAddr >> BLOCK_BITS, blockNum, bytesToWrite));

			for(size_t ii = 0; ii < bytesToWrite; ii++) {
				wire.write(data[ii]);
			}

			int stat = wire.endTransmission(true);
			if (stat == 0) {
				// Success
				eepromAddr += bytesToWrite;
				data += bytesToWrite;
				dataLen -= bytesToWrite;
				result = true;
				break;
			}

			TRACE_DEBUG(("write failed stat=%d tries=%u", stat, writeTries));
			delay(100);
			result = false;
		}

		if (!result) {
			log.info("writeData failed");
			return result;
		}

		// Wait for completion
		for(size_t tries = 1; tries <= 10; tries++) {
			wire.beginTransmission(i2cAddr);
			int stat = wire.endTransmission(true);
			if (stat == 0) {
				break;
			}
			TRACE_DEBUG(("write completion failed stat=%d tries=%u", stat, tries));
		}
	}


	return result;
}

