#ifndef __MCP23008_RK_H
#define __MCP23008_RK_H

#include "Particle.h"

#include <vector>

// Github Repository: https://github.com/rickkas7/MCP23008-RK
// License: MIT

/**
 * @brief Configuration of interrupt output
 * 
 * The INT output of the MCP23008 is optional. If used, you connect it to a hardware GPIO on the MCU.
 * You can use this to monitor for changes in inputs connected to the MCP23008. While you can poll
 * the MCP23008 for these changes, using the interrupt allows the status to be checked by reading
 * a GPIO, which is far faster than doing an I2C transaction.
 */
enum class MCP23008InterruptOutputType {
	/**
	 * @brief Configure INT as normally high, interrupt active low, push-pull
	 */
	ACTIVE_LOW,

	/**
	 * @brief Configure INT as normally low, interrupt active high, push-pull
	 */
	ACTIVE_HIGH,

	/**
	 * @brief Configure INT as open drain, active low, with MCU pull-up resistor
	 * 
	 * This mode is common if you have multiple MCP23008 and you want them all to
	 * share a single MCU interrupt pin. You can connect together multiple
	 * open drain outputs safely with no logic gate. If any one goes low, the
	 * interrupt output goes low.
	 * 
	 * The MCU GPIO is configured as INPUT_PULLUP.
	 */
	OPEN_DRAIN,

	/**
	 * @brief Configure INT as open drain, active low, with no pull-up
	 * 
	 * This is basically the same as OPEN_DRAIN but the MCP GPIO pin is configured as
	 * INPUT instead of INPUT_PULLUP. Note that the pin still needs a pull-up as
	 * the MCP23008 is configured as open-drain! However, you may prefer to use this
	 * mode if you have an external pull-up resistor.
	 */
	OPEN_DRAIN_NO_PULL
};

/**
 * @brief Internal structure used to hold information about configured interrupts
 * 
 * Use attachInterrupt() to add an interrupt handler, which internally creates
 * one of these structures.
 */
struct MCP23008InterruptHandler {
	/**
	 * @brief The MCP23008 pin (0-7)
	 */
	uint16_t pin;

	/**
	 * @brief The interrupt mode, one of:
	 * 
	 * - `RISING`
	 * - `FALLING`
	 * - `CHANGE`
	 * 
	 * These are the same constants used with the Device OS attachInterrupt call.
	 * 
	 * Because the MCP23008 handles rising and falling in a peculiar way that's different
	 * than the STM32 and nRF52 MCUs, the hardware is always configured as change. The
	 * rising and falling cases are generated in software in this library.
	 */
	InterruptMode mode;

	/**
	 * @brief The interrupt handler function or C++11 lambda function.
	 * 
	 * It has this prototype:
	 * 
	 * ```
	 * void hander(bool newState);
	 * ```
	 * 
	 * The newState parameter is the state of the pin after the change that caused
	 * the interrupt. For a RISING interrupt, for example, newState will always
	 * be true.
	 */
	std::function<void(bool)> handler;

	/**
	 * @brief The last state of the GPIO, used to handle the RISING and FALLing cases.
	 */
	bool lastState;
};

/**
 * @brief Class for the MCP23008 I2C GPIO chip
 * 
 * You can connect up to 8 of these to a single I2C interface. You normally create 
 * one of these objects for each chip (at a different I2C address) as a global variable
 * in the main application.
 * 
 * You must call begin() from global setup() before using other library methods!
 */
class MCP23008 {
public:
	/**
	 * @brief Construct the object. Normally you create these as global variables in the main app.
	 * 
	 * @param wire The I2C interface to connect to. This is typically `Wire` (pins D0 and D1) but
	 * could be `Wire1` on the Electron and E Series, or `Wire3` on the Tracker SoM.
	 * 
	 * @param addr the I2C address (0-7) the MCP23008 is configured for using the AD0, AD1, and AD2
	 * pins. Note that this is just the 0-7 part, the actual I2C address will be 0x20 - 0x27.
	 */
	MCP23008(TwoWire &wire, int addr = 0);

	/**
	 * @brief Object destructor
	 * 
	 * As this object is normally allocated as a global variable, it will typically never be
	 * destructed.
	 */
	virtual ~MCP23008();

	/**
	 * @brief Initialize the library - required from setup()!
	 * 
	 * @param callWireBegin Default is true, to call wire.begin() in this method.
	 * 
	 * You must call this from setup() to initialize the hardware. You must not
	 * call this from a global object constructor.
	 */
	void begin(bool callWireBegin = true);

	// Arduino-style API

	/**
	 * @brief Sets the MCP23008 GPIO pin mode
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @param mode The mode, one of the following:
	 * 
	 * - `INPUT` 
	 * - `INPUT_PULLUP`
	 * - `OUTPUT`
	 * 
	 * Note that the MCP23008 does not support input with pull-down or open-drain outputs
	 * for its GPIO.
	 */
	void pinMode(uint16_t pin, PinMode mode);

	/**
	 * @brief Gets the currently set pin mode
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @return One of these constants:
	 * 
	 * - `INPUT` 
	 * - `INPUT_PULLUP`
	 * - `OUTPUT`
	 * 
	 */
	PinMode getPinMode(uint16_t pin);

	/**
	 * @brief Returns true if the pin exists on the device
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @return true if pin is 0 <= pin < 8, otherwise false
	 */
	bool pinAvailable(uint16_t pin) const;

	/**
	 * @brief Sets the output value of the pin
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @param value The value to set the pin to. Typically you use one of:
	 * 
	 * - 0, false, or LOW
	 * - 1, true, or HIGH
	 * 
	 * You must have previously set the pin to OUTPUT mode before using this method.
	 * 
	 * This performs and I2C transaction, so it will be slower that MCU native digitalWrite.
	 */
	void digitalWrite(uint16_t pin, uint8_t value);

	/**
	 * @brief Reads the input value of a pin
	 * 
	 * @return The value, 0 or 1:
	 * 
	 * - 0, false, or LOW
	 * - 1, true, or HIGH
	 * 
	 * This performs and I2C transaction, so it will be slower that MCU native digitalRead.
	 */
	int32_t digitalRead(uint16_t pin);

	/**
	 * @brief Reads all of the pins at once
	 * 
	 * @return A bit mask of the pin values
	 * 
	 * The bit mask is as follows:
	 * 
	 * | Pin   | Mask              |
	 * | :---: | :---------------- |
	 * | 0     | 0b00000001 = 0x01 |
	 * | 1     | 0b00000010 = 0x02 |
	 * | 2     | 0b00000100 = 0x04 |
	 * | 3     | 0b00001000 = 0x08 |
	 * | 4     | 0b00010000 = 0x10 |
	 * | 5     | 0b00100000 = 0x20 |
	 * | 6     | 0b01000000 = 0x40 |
	 * | 7     | 0b10000000 = 0x80 |
	 * 
	 * In other words: `bitMask = (1 << pin)`.
	 * 
	 * This performs and I2C transaction, but all pins are read with a single I2C transaction
	 * so it is faster than calling digitalRead() multiple times. 
	 */
	uint8_t readAllPins();

	/**
	 * @brief Enables MCP23008 interrupt mode
	 * 
	 * @param mcuInterruptPin The MCU pin (D2, A2, etc.) the MCP23008 INT pin is connected
	 * to. This can also be `PIN_INVALID` if you want to use the latching change mode
	 * but do not have the INT pin connected or do not have a spare MCU GPIO.
	 * 
	 * @param outputType The way the INT pin is connected. See MCP23008InterruptOutputType. 
	 * 
	 * Interrupt mode uses the INT output of the MCP23008 to connect to a MCU GPIO pin.
	 * This allows a change (RISING, FALLING, or CHANGE) on one or more GPIO connected
	 * to the expander to trigger the INT line. This is advantageous because the MCU
	 * can poll the INT line much more efficiently than making an I2C transaction to
	 * poll the interrupt register on the MCP23008.
	 * 
	 * Note that this is done from a thread, not using an MCU hardware interrupt. This
	 * is because the I2C transaction requires getting a lock on the Wire object, which
	 * cannot be done from an ISR. Also, because the MCP23008 INT output is latching,
	 * there is no danger of missing it when polling.
	 * 
	 * It's possible for multiple MCP23008 to share a single MCU interrupt line by
	 * using open-drain mode to logically OR them together with no external gate required.
	 * You can also use separate MCU interrupt lines, if you prefer.
	 * 
	 * If you pass `PIN_INVALID` for mcuInterruptPin, outputType is ignored. This mode is
	 * used when you still want to be able to handle latching RISING, FALLING, or CHANGE
	 * handlers but do not have the INT pin connected or do not have spare MCU GPIOs.
	 * This requires an I2C transaction on every thread timeslice (once per millisecond)
	 * so it's not as efficient as using a MCU interrupt pin, but this mode is supported.
	 */
	void enableInterrupts(pin_t mcuInterruptPin, MCP23008InterruptOutputType outputType);

	/**
	 * @brief Attaches an interrupt handler for a pin
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @param mode The interrupt handler mode, one of: `RISING`, `FALLING`, or `CHANGE`.
	 * 
	 * @param handler A function or C++11 lambda with the following prototype:
	 * 
	 * ```
	 * void handlerFunction(bool newState);
	 * ```
	 * 
	 * You must call enableInterrupts() before making this call! 
	 * 
	 * Note that the handler will be called from a thread, not an ISR, so it's not a true
	 * interrupt handler. The reason is that in order to handle the MCP23008 interrupt, more
	 * than one I2C transaction is required. This cannot be easily done at ISR time because
	 * an I2C lock needs to be acquired. It can, however, be easily done from a thread, which
	 * is what is done here. enableInterrupts() starts this thread.
	 * 
	 * Even though handler is called from a thread and not an ISR you should still avoid any
	 * lengthy operations during it, as the thread handles all interrupts on all MCP23008
	 * and blocking it will prevent all other interrupts from being handled.
	 * 
	 * There is also a version of this method that takes a class member function below.
	 * 
	 * If you need to pass additional data ("context") you should instead use a C++11 lambda.
	 * 
	 * You must not call attachInterrupt() from an interrupt handler! If you do so, this function
	 * will deadlock the thread and all interrupt-related functions will stop working.
	 */
	void attachInterrupt(uint16_t pin, InterruptMode mode, std::function<void(bool)> handler);

	/**
	 * @brief Attaches an interupt handler in a class member function to a pin
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @param mode The interrupt handler mode, one of: `RISING`, `FALLING`, or `CHANGE`.
	 * 
	 * @param callback A C++ class member function with following prototype:
	 * 
	 * ```
	 * void MyClass::handler(bool newState);
	 * ```
	 * 
	 * @param instance The C++ object instance ("this") pointer.
	 * 
	 * You typically use it like this:
	 * 
	 * ```
	 * attachInterrupt(2, FALLING, &MyClass::handler, this);
	 * ```
	 * 
	 * You must call enableInterrupts() before making this call! 
	 * 
	 * Note that the handler will be called from a thread, not an ISR, so it's not a true
	 * interrupt handler. The reason is that in order to handle the MCP23008 interrupt, more
	 * than one I2C transaction is required. This cannot be easily done at ISR time because
	 * an I2C lock needs to be acquired. It can, however, be easily done from a thread, which
	 * is what is done here. enableInterrupts() starts this thread.
	 * 
	 * Even though handler is called from a thread and not an ISR you should still avoid any
	 * lengthy operations during it, as the thread handles all interrupts on all MCP23008
	 * and blocking it will prevent all other interrupts from being handled.
	 * 
	 * There is also a version of this method that takes a class member function below.
	 * 
	 * If you need to pass additional data ("context") you should instead use a C++11 lambda.
	 * 
	 * You must not call attachInterrupt() from an interrupt handler! If you do so, this function
	 * will deadlock the thread and all interrupt-related functions will stop working.	 
	 */
	template <typename T>
    void attachInterrupt(uint16_t pin, InterruptMode mode, void (T::*callback)(bool), T *instance) {
        return attachInterrupt(pin, mode, std::bind(callback, instance, std::placeholders::_1));
    };

	/**
	 * @brief Detaches an interrupt handler for a pin
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * You must not call detachInterrupt() from your interrupt handler! If you do so, this function
	 * will deadlock the thread and all interrupt-related functions will stop working.
	 * 
	 * You should avoid attaching and detaching an interrupt excessively as it's a relatively expensive 
	 * operation. You should only attach and detach from loop().
	 */
	void detachInterrupt(uint16_t pin);


	/**
	 * @brief Sets the stack size of the interrupt handler worker thread
	 * 
	 * @param value Size in bytes. Default is 1024.
	 * 
	 * @return Returns *this so you can chain options together, fluent-style.
	 * 
	 * You must call this before the first call to enableInterrupts(). Making the call after will
	 * have no effect. If multiple MCP23008 objects are used (multiple chips) you must make this
	 * call before the first call to enableInterrupts() as all instances share a single thread.
	 */
	MCP23008 &withStackSize(size_t value) { stackSize = value; return *this; };

	/**
	 * @brief Reads an MCP23008 register
	 * 
	 * @param reg The register number, typically one of the constants like `REG_IODIR` (0).
	 * 
	 * @return the value. All MCP23008 register values are 8-bit (uint8_t).
	 * 
	 * There is no way to know if this actually succeeded.
	 */
	uint8_t readRegister(uint8_t reg);

	/**
	 * @brief Writes an MCP23008 register
	 * 
	 * @param reg The register number, typically one of the constants like `REG_IODIR` (0).
	 * 
	 * @param value The value. All MCP23008 register values are 8-bit (uint8_t).
	 * 
	 * @return true if the I2C operation completed successfully or false if not.
	 */
	bool writeRegister(uint8_t reg, uint8_t value);

protected:
	// These are just for reference so you can see which way the pin numbers are laid out
	// (1 << pin) is also a good way to map pin numbers to their bit
	// Do not use these constants for any uint16_t pin variable, those functions require 0 - 7
	// not a bitmask!
	static const uint8_t PIN_0 = 0b00000001; 	//!< bit mask for GP0
	static const uint8_t PIN_1 = 0b00000010; 	//!< bit mask for GP1
	static const uint8_t PIN_2 = 0b00000100; 	//!< bit mask for GP2
	static const uint8_t PIN_3 = 0b00001000; 	//!< bit mask for GP3
	static const uint8_t PIN_4 = 0b00010000; 	//!< bit mask for GP4
	static const uint8_t PIN_5 = 0b00100000; 	//!< bit mask for GP5
	static const uint8_t PIN_6 = 0b01000000; 	//!< bit mask for GP6
	static const uint8_t PIN_7 = 0b10000000; 	//!< bit mask for GP7

	static const uint16_t NUM_PINS = 8; 		//!< Number of GP pins on the MCP23008 (8)

	static const uint8_t REG_IODIR = 0x0; 		//!< MCP23008 register number for IODIR
	static const uint8_t REG_IPOL = 0x1;		//!< MCP23008 register number for IPOL
	static const uint8_t REG_GPINTEN = 0x2;		//!< MCP23008 register number for GPINTEN
	static const uint8_t REG_DEFVAL = 0x3;		//!< MCP23008 register number for DEFVAL
	static const uint8_t REG_INTCON = 0x4;		//!< MCP23008 register number for INTCON
	static const uint8_t REG_IOCON = 0x5;		//!< MCP23008 register number for IOCON
	static const uint8_t REG_GPPU = 0x6;		//!< MCP23008 register number for GPPU
	static const uint8_t REG_INTF = 0x7;		//!< MCP23008 register number for INTF
	static const uint8_t REG_INTCAP = 0x8;		//!< MCP23008 register number for INTCAP
	static const uint8_t REG_GPIO = 0x9;		//!< MCP23008 register number for GPIO
	static const uint8_t REG_OLAT = 0xa;		//!< MCP23008 register number for OLAT

	static const uint8_t DEVICE_ADDR = 0b0100000; //!< The base I2C address for the MCP23008 (0x20)

    /**
     * @brief This class is not copyable
     */
    MCP23008(const MCP23008&) = delete;

    /**
     * @brief This class is not copyable
     */
    MCP23008& operator=(const MCP23008&) = delete;

	/**
	 * @brief Reads an MCP23008 register and masks off a specific bit
	 * 
	 * @param reg The register number, typically one of the constants like `REG_GPIO` (9).
	 * 
	 * @param pin The pin 0 - 7
	 * 
	 * @return the value boolean value
	 * 
	 * For registers that contain a bitmask of all 8 values (like REG_GPIO), reads all
	 * registers and then performs the necessary bit manipulation.
	 * 
	 * There is no way to know if this actually succeeded.
	 */
	bool readRegisterPin(uint8_t reg, uint16_t pin);

	/**
	 * @brief Reads an MCP23008 register, masks off a specific bit, and writes the register back
	 * 
	 * @param reg The register number, typically one of the constants like `REG_GPIO` (9).
	 * 
	 * @param pin The pin 0 - 7
	 * 
	 * @param value The bit value to set (true or false)
	 * 
	 * @return true if the I2C operation completed successfully or false if not.
	 * 
	 * For registers that contain a bitmask of all 8 values (like REG_GPIO), reads all
	 * registers, performs the necessary bit manipulation, and writes the register back.
	 * 
	 * The read/modify/write cycle is done within a single I2C lock()/unlock() pair to
	 * minimize the chance of simultaneous modification.
	 */
	bool writeRegisterPin(uint8_t reg, uint16_t pin, bool value);

	/**
	 * @brief Handles interrupts, used internally
	 * 
	 * This method is called from the worker thread when the MCU interrupt GPIO line
	 * signals an interrupt. It then queries the MCP23008 by I2C to read the
	 * interrupt status and call any handlers for pins that caused interrupt(s).
	 */
	void handleInterrupts();

	/**
	 * @brief Thread function, used internally
	 * 
	 * There is only one thread for all instances of this object
	 */
	static os_thread_return_t threadFunctionStatic(void* param);

	/**
	 * @brief The MCU GPIO the MCP23008 INT pin is connected to 
	 * 
	 * Is `PIN_INVALID` if interrupts are not used. This is set by `enableInterrupts()`.
	 */
	pin_t mcuInterruptPin = PIN_INVALID;

	/**
	 * @brief Vector of interrupt handlers
	 * 
	 * This is used by handleInterrupts() and modified by attachInterrupt() and detachInterrupt().
	 * 
	 * Simultaneous use is prevented by using handlersMutex and std::vector is not
	 * inherently thread-safe.
	 */
	std::vector<MCP23008InterruptHandler*> interruptHandlers;

	/**
	 * @brief Mutex used to prevent simultaneous use of interruptHandlers
	 * 
	 * While technically you can use the vector from two threads at the same time,
	 * you can't modify it from another threads at the same time as it will break
	 * the iterator. Since we only read from one thread (the worker thread), this
	 * isn't a case worth optimizing for.
	 */
	os_mutex_t handlersMutex = 0;

	/**
	 * @brief The I2C interface to use, typically `Wire`
	 */
	TwoWire &wire;

	/**
	 * @brief Which MCP23008 to use (0-7) based on A0, A1, and A2 on the MCP23008
	 * 
	 * This is just 0-7, the (0b0100000 of the 7-bit address is ORed in later)
	 */
	int addr; 

	/**
	 * @brief Default stack size for the worker thread.
	 * 
	 * You can change this using withStackSize() before the first call to enableInterrupts()
	 */
	size_t stackSize = 1024;

	/**
	 * @brief The worker thread object
	 * 
	 * This is NULL before the first call to enableInterrupts(). A single thread is shared
	 * by all instances of this object.
	 */
	static Thread *thread;

	/**
	 * @brief Array of MCP23008 object instances
	 * 
	 * This is modified on the contructor and destructor for MCP23008. It is used from
	 * the worker thread to handle all interrupts across all MCP23008 objects from a
	 * single thread.
	 */
	static std::vector<MCP23008 *> *instances;
};

#endif /* __MCP23008_RK_H */
