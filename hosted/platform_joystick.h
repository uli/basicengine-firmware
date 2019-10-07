// Dummy joystick driver.

#include "Arduino.h"

class Joystick {
public:
	void begin() {
	}

	void setupPins(byte dataPin, byte cmndPin, byte attPin, byte clockPin, byte delay) {
	}

	int read() {
		return 0;
	}
};
