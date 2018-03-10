/*
 SID.cpp - Atmega8 MOS6581 SID Emulator
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
 
/************************************************************************

	Atmega8 MOS6581 SID Emulator

	SID = Sound Interface Device

	This program tries to emulate the sound chip SID of the famous 
	historical C64 Commodore computer.
	The SID emulator includes all registers of the original SID, but 
	some functions may not be implemented yet.
	If you want	to program the SID registers to generate your own sound,
	please refer to the MOS6581 datasheet. The emulator tries to be as
	compatible as possible.

	In the main program there is an interrupt routine which sets the 
	ouptut values for the PWM-Output at 62.5kHz. Therefore the high 
	frequency noise	of the PWM should not be audible to normal people.
	The output is calculated with a 16kHz sample frequency to save 
	processing cycles.
	
	The envelope generators are updated every 1ms. 

	The amplitude value is output as an 8Bit PWM value. 
	The PWM-Output may be directly connected to an audio amplifier.
	
************************************************************************

	Hardware
	
	processor:	ATMEGA8, ATMEGA168
	clock:		16MHz Crystal

	PIN15	PB1/OC1A		8Bit PWM sound output
	PIN19	PB0				test LED

***************************************************************************/

#include <Arduino.h>

#include "SID.h"

// attack, decay, release envelope timings
const static uint16_t AttackRate[16]={2,4,16,24,38,58,68,80,100,250,500,800,1000,3000,5000,8000};
const static uint16_t DecayReleaseRate[16]={6,24,48,72,114,168,204,240,300,750,1500,2400,3000,9000,15000,24000};
	
static uint8_t output;
static Sid_t Sid;
static Oscillator_t osc[OSCILLATORS];

void initialize()
{
}

static int8_t ICACHE_RAM_ATTR wave(Voice_t *voice, uint16_t phase)
{
	int8_t out = 0;
	uint8_t n = phase >> 8;
	uint8_t wavetype = voice->ControlReg;
	
	if(wavetype & SAWTOOTH)  
	{
		out = n - 128;
	}

	if(wavetype & TRIANGLE) 
	{
		if(n&0x80) 
			out = ((n^0xFF)<<1)-128;
		else 
			out = (n<<1)-128;
	}

	if(wavetype & RECTANGLE) 
	{
		if(n > (voice->PW >> 4)) // SID has 12Bit pwm, here we use only 8Bit
			out = 127;
		else 
			out = -127;
	}
		
	return out;
}

static void ICACHE_RAM_ATTR waveforms()
{
	static uint16_t phase[3], sig[3];
	static int16_t temp,temp1;
	static uint8_t i,j,k;
	static uint16_t noise = 0xACE1;
	static uint8_t noise8;
	static uint16_t tempphase;

	// noise generator based on Galois LFSR
	noise = (noise >> 1) ^ (-(noise & 1) & 0xB400u);	
	noise8 = noise>>8;
	
	for(i = 0; i< 3; i++)
	{
		j = (i == 0 ? 2 : i - 1);
		tempphase=phase[i]+osc[i].freq_coefficient; //0.88us
		if(Sid.block.voice[i].ControlReg&NOISE)
		{				
			if((tempphase^phase[i])&0x4000) sig[i]=noise8*osc[i].envelope;			
		}
		else
		{
			if(Sid.block.voice[i].ControlReg&RINGMOD)
			{				
				if(phase[j]&0x8000) 
					sig[i]=osc[i].envelope*-wave(&Sid.block.voice[i],phase[i]);
				else 
					sig[i]=osc[i].envelope*wave(&Sid.block.voice[i],phase[i]);
			}
			else 
			{
				if(Sid.block.voice[i].ControlReg&SID_SYNC)
				{
					if(tempphase < phase[j]) 
						phase[i] = 0;
				}
				else 
					sig[i]=osc[i].envelope*wave(&Sid.block.voice[i],phase[i]); //2.07us
			}
		}
		phase[i]=tempphase;
	}
	
	// voice filter selection
	temp=0; // direct output variable
	temp1=0; // filter output variable
	if(Sid.block.RES_Filt&FILT1) temp1+=sig[0];
	else temp+=sig[0];
	if(Sid.block.RES_Filt&FILT2) temp1+=sig[1];
	else temp+=sig[1];
	if(Sid.block.RES_Filt&FILT3) temp1+=sig[2];
	else if(!(Sid.block.Mode_Vol&VOICE3OFF))temp+=sig[2]; // voice 3 with special turn off bit

	//filterOutput = IIR2((struct IIR_filter*)&filter04_06, filterInput);
	//IIR2(filter04_06, temp1);
	k=(temp>>8)+128;
	k+=temp1>>10; // no real filter implemeted yet
	
	output = k; // Output to PWM
}


static void ICACHE_RAM_ATTR envelopes()
{
	uint8_t n;
	uint8_t controll_regadr[3]={4,11,18};
	// if gate is ONE then the attack,decay,sustain cycle begins
	// if gate switches to zero the sound decays
	for(n=0;n<OSCILLATORS;n++)
	{
		if(Sid.sidregister[controll_regadr[n]]&GATE) // if gate set then attack,decay,sustain
		{
			if(osc[n].attackdecay_flag) 
			{	// if attack cycle
				osc[n].amp+=osc[n].m_attack;
				if(osc[n].amp>MAXLEVEL)
				{
					osc[n].amp=MAXLEVEL;
					osc[n].attackdecay_flag=false; // if level reached, then switch to decay
				}
			}
			else // decay cycle
			{
				if(osc[n].amp>osc[n].level_sustain)
				{
					osc[n].amp-=osc[n].m_decay;
					if(osc[n].amp<osc[n].level_sustain) osc[n].amp=osc[n].level_sustain;
				}

			} 
		}
		else // if gate flag is not set then release
		{
			osc[n].attackdecay_flag=true; // at next attack/decay cycle start wiht attack
			if(osc[n].amp>0)
			{
				osc[n].amp-=osc[n].m_release;
				if(osc[n].amp<0) osc[n].amp=0;
			}			
		}
		osc[n].envelope=osc[n].amp>>8;
	}
}

/************************************************************************

	interrupt routine timer 2 16kHz
	- calculate waverform phases
	- calculate waveforms
	- calculate attack decay release (1kHz)
	
************************************************************************/
uint8_t ICACHE_RAM_ATTR SID::getSample()
{
	static uint8_t mscounter = 0;
	waveforms(); //~22us
	
	if(mscounter++ >= MSCOUNT)
	{
		envelopes(); //~16us
		mscounter = 0;
	}
	return output;
}

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

void SID::begin()
{
	initialize();
		
	//initialize SID-registers	
	Sid.sidregister[6]=0xF0;
	Sid.sidregister[13]=0xF0;
	Sid.sidregister[20]=0xF0;

	
	// set all amplitudes to zero
	for(int n=0;n<OSCILLATORS;n++) {
		osc[n].attackdecay_flag=true;
		setenvelope(&Sid.block.voice[n]);
		osc[n].amp=0;
	}
}


// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries
/************************************************************************
	
	uint8_t set_sidregister(uint8_t regnum, uint8_t value)

	The registers of the virtual SID are set by this routine.
	For some registers it is necessary to transform the SID-register
	values to some internal settings of the emulator. 
	To select this registers and to start the calculation, the switch/
	case statement is used.
	For instance: If setting the SID envelope register, new attach, decay
	sustain times are calculated.
	If an invalid register is requested the returned value will be 0.

	4.2007 ch

************************************************************************/
uint8_t ICACHE_RAM_ATTR SID::set_register(uint8_t regnum, uint8_t value)
{
	if(regnum>NUMREGISTERS-1) 
		return 0;
		
	Sid.sidregister[regnum]=value;

	switch(regnum)
	{
		//voice1
		case 1:
			osc[0].freq_coefficient=((uint16_t)Sid.sidregister[0]+((uint16_t)Sid.sidregister[1]<<8))>>2;
			break;
		case 5: setenvelope(&Sid.block.voice[0]);break;
		case 6: setenvelope(&Sid.block.voice[0]);break;
		
		//voice2
		case 8:
			osc[1].freq_coefficient=((uint16_t)Sid.sidregister[7]+((uint16_t)Sid.sidregister[8]<<8))>>2;
			break;
		case 12: setenvelope(&Sid.block.voice[1]);break;
		case 13: setenvelope(&Sid.block.voice[1]);break;		
		
		//voice3
		case 15:
			osc[2].freq_coefficient=((uint16_t)Sid.sidregister[14]+((uint16_t)Sid.sidregister[15]<<8))>>2;
			break;
		case 19: setenvelope(&Sid.block.voice[2]);break;
		case 20: setenvelope(&Sid.block.voice[2]);break;			
	}	
	return 1;
}

/************************************************************************
	
	uint8_t get_sidregister(uint8_t regnum)

	The registers of the virtual SID are read by this routine.
	If an invalid register is requested it returns zero.

************************************************************************/
uint8_t SID::get_register(uint8_t regnum)
{
	if(regnum>NUMREGISTERS-1)
		return 0;
    return Sid.sidregister[regnum];
}

// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

uint8_t SID::get_wavenum(Voice_t *voice)
{
	uint8_t n;

	if(voice==&Sid.block.voice[0]) n=0;
	if(voice==&Sid.block.voice[1]) n=1;
	if(voice==&Sid.block.voice[2]) n=2;
	
	return n;
}

void SID::setfreq(Voice_t *voice,uint16_t freq)
{
	uint32_t templong;
	uint8_t n;

	n=get_wavenum(voice);
	
	templong=freq;
	osc[n].freq_coefficient=templong*4000/SAMPLEFREQ;
}


void SID::setenvelope(Voice_t *voice)
{
	uint8_t n;
	
	n=get_wavenum(voice);
	osc[n].attackdecay_flag=true;

	osc[n].level_sustain=(voice->SustainRelease>>4)*SUSTAINFACTOR;
	osc[n].m_attack=MAXLEVEL/AttackRate[voice->AttackDecay>>4];
	osc[n].m_decay=(MAXLEVEL-osc[n].level_sustain*SUSTAINFACTOR)/DecayReleaseRate[voice->AttackDecay&0x0F];
	osc[n].m_release=(osc[n].level_sustain)/DecayReleaseRate[voice->SustainRelease&0x0F];
}
