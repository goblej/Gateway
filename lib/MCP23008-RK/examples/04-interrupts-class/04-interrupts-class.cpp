#include "Particle.h"

#include "MCP23008-RK.h"

SerialLogHandler logHandler;

MCP23008 gpio(Wire, 0);

uint16_t SWITCH_PIN = 2; // GP2

class MyClass {
public:
    MyClass(pin_t pin);
    virtual ~MyClass();

    void setup();

    void callback(bool newState);

protected:
    pin_t pin;
};

MyClass::MyClass(pin_t pin) : pin(pin) {
    
}

MyClass::~MyClass() {
    
}

void MyClass::setup() {
    // OK to call setup() more than once (if there are multiple instances of MyClass)
    gpio.attachInterrupt(SWITCH_PIN, FALLING, &MyClass::callback, this);
}

void MyClass::callback(bool newState) {
    Log.info("pin=%d newState=%d", pin, newState);
}

MyClass myClass(D2);


void setup() {
    waitFor(Serial.isConnected, 15000);

	gpio.begin();

    // When using interrupt mode, you need to assocate a physical MCU pin as an interrupt pin
    // from the MCP23008 INT output
    gpio.enableInterrupts(A3, MCP23008InterruptOutputType::OPEN_DRAIN);

    gpio.pinMode(SWITCH_PIN, INPUT_PULLUP);
    
    myClass.setup();
}

void loop() {
}
