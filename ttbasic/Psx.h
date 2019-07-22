/*  PSX Controller Decoder Library (Psx.h)
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


#ifndef Psx_h
#define Psx_h

#include "Arduino.h"

// Button Hex Representations:
#define psxLeftShift	0
#define psxDownShift	1
#define psxRightShift	2
#define psxUpShift	3
#define psxStrtShift	4
#define psxSlctShift	5
#define psxErrorShift	16

#define psxLeft		(1 << psxLeftShift)
#define psxDown		(1 << psxDownShift)
#define psxRight	(1 << psxRightShift)
#define psxUp		(1 << psxUpShift)
#define psxStrt		(1 << psxStrtShift)
#define psxSlct		(1 << psxSlctShift)
#define psxError	(1 << psxErrorShift)

#define psxSquShift	8
#define psxXShift	9
#define psxOShift	10
#define psxTriShift	11
#define psxR1Shift	12
#define psxL1Shift	13
#define psxR2Shift	14
#define psxL2Shift	15

#define psxSqu		(1 << psxSquShift)
#define psxX		(1 << psxXShift)
#define psxO		(1 << psxOShift)
#define psxTri		(1 << psxTriShift)
#define psxR1		(1 << psxR1Shift)
#define psxL1		(1 << psxL1Shift)
#define psxR2		(1 << psxR2Shift)
#define psxL2		(1 << psxL2Shift)

class Psx
{
	public:
		Psx();
		void setupPins(byte , byte , byte , byte , byte );		// (Data Pin #, CMND Pin #, ATT Pin #, CLK Pin #, Delay)
															// Delay is how long the clock goes without changing state
															// in Microseconds. It can be lowered to increase response,
															// but if it is too low it may cause glitches and have some
															// keys spill over with false-positives. A regular PSX controller
															// works fine at 50 uSeconds.
															
		int read();								// Returns the status of the button presses in an unsignd int.
															// The value returned corresponds to each key as defined above.
		
	private:
		byte shift(byte _dataOut);

		byte _dataPin;
		byte _cmndPin;
		byte _attPin;
		byte _clockPin;
		
		byte _delay;
		uint32_t _last_failed;
		int _last_read;
};

#endif
