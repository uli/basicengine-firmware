extern "C" {
#include <nosdki2s.h>
}
#include <sound.h>

extern unsigned int i2sData[2][I2S_BUFLEN];

void slc_isr(void *userdata, uint8_t *stream, int len) {
#if 0
	sound.render();

	int i;
	for (i = 0; i < len; ++i) {
		stream[i] = nosdk_i2s_curr_buf[i];
	}
	if (aud_fp)
	  fwrite(stream, len, 1, aud_fp);
        total_samples += len;

	cubu ^= 1;
	nosdk_i2s_curr_buf = i2sData[cubu];
	nosdk_i2s_curr_buf_pos = 0;
#endif
}

int sound_reinit_rate = 0;

extern "C" void InitI2S(uint32_t samplerate)
{
	sound_reinit_rate = samplerate;
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
