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
