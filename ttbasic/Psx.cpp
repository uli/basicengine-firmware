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

Psx::Psx()
{
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
	vs23.digitalWrite(dataPin, HIGH);	// Turn on internal pull-up
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


unsigned int Psx::read()
{
	vs23.digitalWrite(_attPin, LOW);

	shift(0x01);
	shift(0x42);
	shift(0xFF);

	_data1 = ~shift(0xFF);
	_data2 = ~shift(0xFF);

	vs23.digitalWrite(_attPin, HIGH);

	_dataOut = (_data2 << 8) | _data1;

	return _dataOut;
}
