#include <nosdki2s.h>

extern unsigned int i2sData[2][I2S_BUFLEN];

void InitI2S(uint32_t samplerate)
{
	nosdk_i2s_clear_buf();
	nosdk_i2s_curr_buf = i2sData[0];
}

void SendI2S()
{
}

const uint32_t fakePwm[] = {
	  0,   8,  16,  25,  33,  41,  49,  58,
	 66,  74,  82,  90,  99, 107, 115, 123,
	132, 140, 148, 156, 165, 173, 181, 189,
	197, 206, 214, 222, 230, 239, 247, 255,
};
