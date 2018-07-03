#include "ttconfig.h"
#include "nosdki2s.h"
#ifdef ESP8266_NOWIFI
#include <hw/pin_mux_register.h>
#include <hw/esp8266.h>
#include <hw/eagle_soc.h>
#else
#include <eagle_soc.h>
#define IOMUX_BASE ((volatile uint32_t *)PERIPHS_IO_MUX)
#endif

#define pico_i2c_writereg rom_i2c_writeReg
//XXX TODO:  Do we need to worry about "output" here?
#define nosdk8266_configio( port, FUNC, pd, pu ) \
	IOMUX_BASE[(port-PERIPHS_IO_MUX)/4] = ((((FUNC&BIT2)<<2)|(FUNC&0x3))<<PERIPHS_IO_MUX_FUNC_S) | (pu<<7) | (pd<<6);

#if MAIN_MHZ==52 || MAIN_MHZ==104
#error You cannot operate the I2S bus without the PLL enabled. Select another clock frequency.
#endif

// I2S frequency dividers. Base clock appears to be 160 MHz.
#define WS_I2S_BCK 6

volatile uint32_t * const DR_REG_I2S_BASEL = (volatile uint32_t*)0x60000e00;
volatile uint32_t * const DR_REG_SLC_BASEL = (volatile uint32_t*)0x60000B00;

#if defined(ESP8266_NOWIFI) && !defined(HOSTED)
extern char print_mem_buf[];
struct sdio_queue i2sBufDesc[2] = {
	{ .owner = 1, .eof = 1, .sub_sof = 0, .datalen = I2S_BUFLEN*4,  .blocksize = I2S_BUFLEN*4, .buf_ptr = (uint32_t)print_mem_buf, .next_link_ptr = (uint32_t)&i2sBufDesc[1] },
	{ .owner = 1, .eof = 1, .sub_sof = 0, .datalen = I2S_BUFLEN*4,  .blocksize = I2S_BUFLEN*4, .buf_ptr = (uint32_t)(print_mem_buf+I2S_BUFLEN*4), .next_link_ptr = (uint32_t)&i2sBufDesc[0] },
};
#else
static unsigned int i2sData[2][I2S_BUFLEN];
struct sdio_queue i2sBufDesc[2] = {
	{ .owner = 1, .eof = 1, .sub_sof = 0, .datalen = I2S_BUFLEN*4,  .blocksize = I2S_BUFLEN*4, .buf_ptr = (uint32_t)i2sData[0], .next_link_ptr = (uint32_t)&i2sBufDesc[1] },
	{ .owner = 1, .eof = 1, .sub_sof = 0, .datalen = I2S_BUFLEN*4,  .blocksize = I2S_BUFLEN*4, .buf_ptr = (uint32_t)i2sData[1], .next_link_ptr = (uint32_t)&i2sBufDesc[0] },
};
#endif

volatile int isrs = 0;

#define I2S_INTERRUPTS 1

#if I2S_INTERRUPTS

volatile uint32_t *nosdk_i2s_curr_buf;
volatile uint32_t nosdk_i2s_curr_buf_pos;

#if !defined(ENABLE_GDBSTUB) && !defined(HOSTED)
LOCAL ICACHE_RAM_ATTR void slc_isr(void) {
	struct sdio_queue *finished;
	SLC_INT_CLRL = 0xffffffff;

	finished = (struct sdio_queue*) SLC_RX_EOF_DES_ADDR;
	nosdk_i2s_curr_buf = (uint32_t *)finished->buf_ptr;
	nosdk_i2s_curr_buf_pos = 0;

	isrs++;
}
#endif

#endif

#if defined(ESP8266_NOWIFI) && !defined(HOSTED)
#include <string.h>
void nosdk_i2s_clear_buf()
{
	memset(print_mem_buf, 0xaa, I2S_BUFLEN * 4 * 2);
	((uint32_t *)print_mem_buf)[I2S_BUFLEN * 2] = I2S_BUF_GUARD;
}
#else
void nosdk_i2s_clear_buf()
{
	for( int i = 0; i < I2S_BUFLEN; i++ )
	{
		i2sData[0][i] = 0;
		i2sData[1][i] = 0;
	}
}
#endif

#ifndef HOSTED
void InitI2S(uint32_t samplerate)
{
#ifdef ENABLE_GDBSTUB
	return;
#else
	DR_REG_SLC_BASEL[4] = 0;  //Reset DMA
	SLC_CONF0L = (1<<SLC_MODE_S);		//Configure DMA
	SLC_RX_DSCR_CONFL = SLC_INFOR_NO_REPLACE|SLC_TOKEN_NO_REPLACE;

	//Set address for buffer descriptor.
	SLC_RX_LINKL = ((uint32)&i2sBufDesc[0]) & SLC_RXLINK_DESCADDR_MASK;

	nosdk_i2s_clear_buf();

#if I2S_INTERRUPTS

	//Attach the DMA interrupt
	ets_isr_attach(ETS_SLC_INUM, slc_isr, NULL);
	//Enable DMA operation intr
	SLC_INT_ENAL = SLC_RX_EOF_INT_ENA; //Select the interrupt.

	//clear any interrupt flags that are set
	SLC_INT_CLRL = 0xffffffff;
	///enable DMA intr in cpu
	ets_isr_unmask(1<<ETS_SLC_INUM);

#endif
	//Init pins to i2s functions
	nosdk8266_configio(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA, 0, 0);
	// Other I2S pins are used for other functions, so we don't configure them.
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_I2SO_BCK);

	//Enable clock to i2s subsystem
	//From 13 to 93, reconfigure the BBPLL to output i2c_bbpll_en_audio_clock_out
	//Code was originally: //i2c_writeReg_Mask_def(i2c_bbpll, i2c_bbpll_en_audio_clock_out, 1);
	pico_i2c_writereg(103,4,4,0x93);

	//Reset I2S subsystem
	I2SCONFL = I2S_I2S_RESET_MASK;
	I2SCONFL = 0;

	//Select 16bits per channel (FIFO_MOD=0)
	I2S_FIFO_CONFL = I2S_I2S_DSCR_EN; 	//Enable DMA in i2s subsystem

#if I2S_INTERRUPTS

	//Clear int
	I2SINT_CLRL = 0;  //Clear interrupts in I2SINT_CLR
#endif
	//Configure the I2SConf mode.
	uint32_t ws_i2s_div = 160000000UL /	// base clock
			      32 / 		// 32 bits per sample
			      WS_I2S_BCK / 	// base clock divider
			      samplerate;
	I2SCONFL = I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_RECE_SLAVE_MOD|
						I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|
						(((WS_I2S_BCK)&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|
						(((ws_i2s_div)&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S);


	//enable int
	I2SINT_ENAL = I2S_I2S_RX_REMPTY_INT_ENA|I2S_I2S_RX_TAKE_DATA_INT_ENA;

	//Start transmission
	I2SCONFL |= I2S_I2S_TX_START;
#endif	// ENABLE_GDBSTUB
}

void SendI2S()
{
	SLC_RX_LINKL = SLC_RXLINK_STOP;
	SLC_RX_LINKL = (((uint32)&i2sBufDesc[0]) & SLC_RXLINK_DESCADDR_MASK) | SLC_RXLINK_START;
}
#endif	// !HOSTED
