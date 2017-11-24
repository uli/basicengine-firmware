/*
 SID.h - Atmega8 MOS6581 SID Emulator
 Copyright (c) 2007 Christoph Haberer, christoph(at)roboterclub-freiburg.de
 Arduino Library Conversion by Mario Patino, cybernesto(at)gmail.com
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

// ensure this library description is only included once
#ifndef SID_h
#define SID_h

#include <inttypes.h>

#define NUMREGISTERS 29
#define OSCILLATORS 3
#define MAXLEVEL ( 0xFFFF / OSCILLATORS )
#define SUSTAINFACTOR ( MAXLEVEL / 15 )

#define SAMPLEFREQ 16000L
#define SAMPLERATECOUNT (F_CPU/(8*SAMPLEFREQ)-1)

#define ENVELOPE_FREQ 1000L  
#define MSCOUNT (SAMPLEFREQ/ENVELOPE_FREQ-1)


// SID Registers
#define VOICE1	0
#define VOICE2	7
#define VOICE3	14
#define CONTROLREG 4
#define ATTACKDECAY 5
#define SUSTAINRELEASE 6

// SID voice control register bits
#define GATE (1<<0)
#define SID_SYNC (1<<1)
#define RINGMOD (1<<2)
#define TEST (1<<3)		// not implemented
#define TRIANGLE (1<<4)
#define SAWTOOTH (1<<5)
#define RECTANGLE (1<<6)
#define NOISE (1<<7)

// SID RES/FILT ( reg.23 )
#define FILT1 (1<<0)
#define FILT2 (1<<1)
#define FILT3 (1<<2)
// SID MODE/VOL ( reg.24 )  
#define VOICE3OFF (1<<7)

typedef struct
{
	uint16_t	Freq;			// Frequency: FreqLo/FreqHi
	uint16_t	PW;				// PulseWidth: PW LO/HI only 12 bits used in SID
	uint8_t		ControlReg;		// NOISE,RECTANGLE,SAWTOOTH,TRIANGLE,TEST,RINGMOD,SYNC,GATE
	uint8_t		AttackDecay;	// bit0-3 decay, bit4-7 attack
	uint8_t		SustainRelease;	// bit0-3 release, bit4-7 sustain
} Voice_t;

typedef struct
{
	Voice_t voice[3];
	uint16_t FC;		// not implemented
	uint8_t RES_Filt;	// partly implemented
	uint8_t Mode_Vol;	// partly implemented
	uint8_t POTX;		// not implemented
	uint8_t POTY;		// not implemented
	uint8_t OSC3_Random;// not implemented
	uint8_t ENV3;		// not implemented
} Blocks_t;

typedef union
{
	Blocks_t block;
	uint8_t sidregister[NUMREGISTERS];
} Sid_t;

typedef struct
{
	uint16_t freq_coefficient;
	uint8_t envelope;
	uint16_t m_attack;
	uint16_t m_decay;
	uint16_t m_release;
	uint8_t	attackdecay_flag;
	int16_t	level_sustain;	
	int16_t amp;
} Oscillator_t;

// library interface description
class SID
{
  // user-accessible "public" interface
  public:
	void begin();
    uint8_t set_register(uint8_t regnum, uint8_t value);
    uint8_t get_register(uint8_t regnum);
    uint8_t getSample();
  // library-accessible "private" interface
  private:
    uint8_t get_wavenum(Voice_t *voice);
	void setfreq(Voice_t *voice,uint16_t freq);
	void init_waveform(Voice_t *voice);
	void setenvelope(Voice_t *voice);
};

#endif

