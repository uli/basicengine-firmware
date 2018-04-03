/*  PSX Controller Decoder Library (Psx.cpp)
	Written by: Kevin Ahrendt June 22nd, 2008
	
	Controller protocol implemented using Andrew J McCubbin's analysis.
	http://www.gamesx.com/controldata/psxcont/psxcont.htm
	
	Shift command is based on tutorial examples for ShiftIn and ShiftOut
	functions both written by Carlyn Maw and Tom Igoe
	http://www.arduino.cc/en/Tutorial/ShiftIn
	http://www.arduino.cc/en/Tutorial/ShiftOut

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Psx.h"
#include "vs23s010.h"

//#define DEBUG_PSX

#ifdef DEBUG_PSX
#include "basic.h"
#define dbg_psx dbg_printf
#else
#define dbg_psx(x...)
#endif

Psx::Psx()
{
	_last_read = 0;
}

byte Psx::shift(byte _dataOut)							// Does the actual shifting, both in and out simultaneously
{
	_temp = 0;
	_dataIn = 0;

	for (_i = 0; _i <= 7; _i++)
	{
		
		
		if ( _dataOut & (1 << _i) ) vs23.digitalWrite(_cmndPin, HIGH);	// Writes out the _dataOut bits
		else vs23.digitalWrite(_cmndPin, LOW);

		vs23.digitalWrite(_clockPin, LOW);
		
		delayMicroseconds(_delay);

		_temp = vs23.digitalRead(_dataPin);					// Reads the data pin
		if (_temp)
		{
			_dataIn = _dataIn | (B10000000 >> _i);		// Shifts the read data into _dataIn
		}

		vs23.digitalWrite(_clockPin, HIGH);
		delayMicroseconds(_delay);
	}
	return _dataIn;
}


void Psx::setupPins(byte dataPin, byte cmndPin, byte attPin, byte clockPin, byte delay)
{
	vs23.pinMode(dataPin, INPUT);

	// We don't have an internal pull-up. (In fact, we have an internal
	// pull-down which we override with a strong external pull-up...)
	//vs23.digitalWrite(dataPin, HIGH);	// Turn on internal pull-up

	_dataPin = dataPin;

	vs23.pinMode(cmndPin, OUTPUT);
	_cmndPin = cmndPin;

	vs23.pinMode(attPin, OUTPUT);
	_attPin = attPin;
	vs23.digitalWrite(_attPin, HIGH);

	vs23.pinMode(clockPin, OUTPUT);
	_clockPin = clockPin;
	vs23.digitalWrite(_clockPin, HIGH);
	
	_delay = delay;
}

#define MAX_RETRIES 3
#define CONFIRMATIONS 2

int Psx::read()
{
    byte data1, data2;
    int data_out;
    int retries;

    if (_last_read < 0 && millis() < _last_failed + 1000) {
      _last_failed = millis();
      return psxError;
    }

    _last_read = -1;
#ifdef DEBUG_PSX
    uint32_t now = micros();
#endif
    uint32_t spiclk = vs23.getSpiClock();
    vs23.setSpiClockMax();

    // We want more than one consecutive read to yield the same data before
    // we trust it to be correct.
    for (int confirmations = 0; confirmations < CONFIRMATIONS; ) {
      // If we don't get the right magic byte back, we retry a few times
      // before giving up.
      for (retries = 0; retries < MAX_RETRIES; ++retries) {
	vs23.digitalWrite(_attPin, LOW);

	shift(0x01);
	shift(0x42);	// returns report type
	byte magic = shift(0xFF);

	data1 = ~shift(0xFF);
	data2 = ~shift(0xFF);

	vs23.digitalWrite(_attPin, HIGH);

	data_out = (data2 << 8) | data1;

        if (magic == 0x5a) {
          if (data_out == _last_read)
            confirmations++;
          _last_read = data_out;
          break;
        }

        // Not the right magic byte, try again.
        _last_read = -1;

        // wait some time before doing another read
        // XXX: wild guess
	delayMicroseconds(_delay*2);
      }

      if (retries == 3)	{
        // exhausted all retries, there might be no controller here
        _last_failed = millis();
        return psxError;
      }

      // wait some time before doing another read
      // XXX: wild guess
      delayMicroseconds(_delay*2);
    }

    vs23.setSpiClock(spiclk);
    dbg_psx("psxr %d\r\n", micros() - now);
    return data_out;
}
