// FILE: TNTSC.cpp
// Arduino STM32 用 NTSCビデオ出力ライブラリ by たま吉さん
// 作成日 2017/02/20, Blue Pillボード(STM32F103C8)にて動作確認
// 更新日 2017/02/27, delay_frame()の追加
// 更新日 2017/02/27, フック登録関数追加
// 更新日 2017/03/03, 解像度モード追加
// 更新日 2017/04/05, クロック48MHz対応
// 更新日 2017/04/18, SPI割り込みの廃止(動作確認中)
// 更新日 2017/04/27, NTSC走査線数補正関数追加
// 更新日 2017/04/30, SPI1,SPI2の選択指定を可能に修正
// 更新日 2017/06/25, 外部確保VRAMの指定を可能に修正

#include <TNTSC.h>
#include <SPI.h>

#define gpio_write(pin,val) gpio_write_bit(PIN_MAP[pin].gpio_device, PIN_MAP[pin].gpio_bit, val)

#define PWM_CLK PA1         // 同期信号出力ピン(PWM)
#define DAT PA7             // 映像信号出力ピン
#define NTSC_S_TOP 3        // 垂直同期開始ライン
#define NTSC_S_END 5        // 垂直同期終了ライン
#define NTSC_VTOP 30        // 映像表示開始ライン
#define IRQ_PRIORITY  2     // タイマー割り込み優先度
#define MYSPI1_DMA_CH DMA_CH3 // SPI1用DMAチャンネル
#define MYSPI2_DMA_CH DMA_CH5 // SPI2用DMAチャンネル
#define MYSPI_DMA DMA1        // SPI用DMA

// 画面解像度別パラメタ設定
typedef struct  {
  uint16_t width;   // 画面横ドット数
  uint16_t height;  // 画面縦ドット数
  uint16_t ntscH;   // NTSC画面縦ドット数
  uint16_t hsize;   // 横バイト数
  uint8_t  flgHalf; // 縦走査線数 (0:通常 1:半分)
  uint32_t spiDiv;  // SPIクロック分周
} SCREEN_SETUP;

#if F_CPU == 72000000L
#define NTSC_TIMER_DIV 3 // システムクロック分周 1/3
const SCREEN_SETUP screen_type[] __FLASH__ {
  { 112, 108, 216, 14, 1, SPI_CLOCK_DIV32 }, // 112x108
  { 224, 108, 216, 28, 1, SPI_CLOCK_DIV16 }, // 224x108 
  { 224, 216, 216, 28, 0, SPI_CLOCK_DIV16 }, // 224x216
  { 448, 108, 216, 56, 1, SPI_CLOCK_DIV8  }, // 448x108 
  { 448, 216, 216, 56, 0, SPI_CLOCK_DIV8  }, // 448x216 
};
#elif  F_CPU == 48000000L
#define NTSC_TIMER_DIV 2 // システムクロック分周 1/2
const SCREEN_SETUP screen_type[] __FLASH__ {
  { 128,  96, 192, 16, 1, SPI_CLOCK_DIV16 }, // 128x96
  { 256,  96, 192, 32, 1, SPI_CLOCK_DIV8  }, // 256x96 
  { 256, 192, 192, 32, 0, SPI_CLOCK_DIV8  }, // 256x192
  { 512,  96, 192, 64, 1, SPI_CLOCK_DIV4  }, // 512x96 
  { 512, 192, 192, 64, 0, SPI_CLOCK_DIV4  }, // 512x192 
  { 128, 108, 216, 16, 1, SPI_CLOCK_DIV16 }, // 128x108
  { 256, 108, 216, 32, 1, SPI_CLOCK_DIV8  }, // 256x108 
  { 256, 216, 216, 32, 0, SPI_CLOCK_DIV8  }, // 256x216
  { 512, 108, 216, 64, 1, SPI_CLOCK_DIV4  }, // 512x108 
  { 512, 216, 216, 64, 0, SPI_CLOCK_DIV4  }, // 512x216 
};
#endif

#define NTSC_LINE (262+0)                     // 画面構成走査線数(一部のモニタ対応用に2本に追加)
#define SYNC(V)  gpio_write(PWM_CLK,V)        // 同期信号出力(PWM)
static uint8_t* vram;                         // ビデオ表示フレームバッファ
static volatile uint8_t* ptr;                 // ビデオ表示フレームバッファ参照用ポインタ
static volatile int count=1;                  // 走査線を数える変数

static void (*_bktmStartHook)() = NULL;       // ブランキング期間開始フック
static void (*_bktmEndHook)()  = NULL;        // ブランキング期間終了フック

static uint8_t  _screen;
static uint16_t _width;
static uint16_t _height;
static uint16_t _ntscHeight;
static uint16_t _vram_size;
static uint16_t _ntsc_line = NTSC_LINE;
static uint16_t _ntsc_adjust = 0;
static uint8_t  _spino = 1;
static dma_channel  _spi_dma_ch = MYSPI1_DMA_CH;
static dma_dev* _spi_dma    = MYSPI_DMA;
static SPIClass* pSPI;

uint16_t TNTSC_class::width()  {return _width;;} ;
uint16_t TNTSC_class::height() {return _height;} ;
uint16_t TNTSC_class::vram_size() { return _vram_size;};
uint16_t TNTSC_class::screen() { return _screen;};


 // ブランキング期間開始フック設定
void TNTSC_class::setBktmStartHook(void (*func)()) {
  _bktmStartHook = func;
}

// ブランキング期間終了フック設定
void TNTSC_class::setBktmEndHook(void (*func)()) {
  _bktmEndHook = func;
}

// DMA用割り込みハンドラ(データ出力をクリア)
void TNTSC_class::DMA1_CH3_handle() {
  while(pSPI->dev()->regs->SR & SPI_SR_BSY);
    pSPI->dev()->regs->DR = 0;
}

// DMAを使ったデータ出力
void TNTSC_class::SPI_dmaSend(uint8_t *transmitBuf, uint16_t length) {
  dma_setup_transfer( 
    _spi_dma, _spi_dma_ch,  // SPI1用DMAチャンネル指定
    &pSPI->dev()->regs->DR, // 転送先アドレス    ：SPIデータレジスタを指定
    DMA_SIZE_8BITS,         // 転送先データサイズ : 1バイト
    transmitBuf,            // 転送元アドレス     : SRAMアドレス
    DMA_SIZE_8BITS,         // 転送先データサイズ : 1バイト
    DMA_MINC_MODE|          // フラグ: サイクリック
    DMA_FROM_MEM |          //         メモリから周辺機器、転送完了割り込み呼び出しあり 
    DMA_TRNS_CMPLT          //         転送完了割り込み呼び出しあり  */
  );
  dma_set_num_transfers(_spi_dma, _spi_dma_ch, length); // 転送サイズ指定
  dma_enable(_spi_dma, _spi_dma_ch);  // DMA有効化
}

// ビデオ用データ表示(ラスタ出力）
void TNTSC_class::handle_vout() {
  if (count >=NTSC_VTOP && count <=_ntscHeight+NTSC_VTOP-1) {  	

    SPI_dmaSend((uint8_t *)ptr, screen_type[_screen].hsize);
  	//pSPI->dmaSend((uint8_t *)ptr, screen_type[_screen].hsize,1);
  	if (screen_type[_screen].flgHalf) {
      if ((count-NTSC_VTOP) & 1) 
      ptr+= screen_type[_screen].hsize;
    } else {
      ptr+=screen_type[_screen].hsize;
    }
  }
	
  // 次の走査線用同期パルス幅設定
  if(count >= NTSC_S_TOP-1 && count <= NTSC_S_END-1){
    // 垂直同期パルス(PWMパルス幅変更)
    TIMER2->regs.adv->CCR2 = 1412;
  } else {
    // 水平同期パルス(PWMパルス幅変更)
    TIMER2->regs.adv->CCR2 = 112;
  }

   count++; 
  if( count > _ntsc_line ){
    count=1;
    ptr = vram;    
  } 

}

void TNTSC_class::adjust(int16_t cnt) {
  _ntsc_adjust = cnt;
  _ntsc_line = NTSC_LINE+cnt;
}
	
// NTSCビデオ表示開始
//void TNTSC_class::begin(uint8_t mode) {
void TNTSC_class::begin(uint8_t mode, uint8_t spino, uint8_t* extram) {
   // スクリーン設定
   _screen = mode <=4 ? mode: SC_DEFAULT;
   _width  = screen_type[_screen].width;
   _height = screen_type[_screen].height;   
   _vram_size  = screen_type[_screen].hsize * _height;
   _ntscHeight = screen_type[_screen].ntscH;
   _spino = spino;
   flgExtVram = false;
  
   if (extram) {
     vram = extram;
     flgExtVram = true;
   } else {
     vram = (uint8_t*)malloc(_vram_size);  // ビデオ表示フレームバッファ
   }
   cls();
   ptr = vram;  // ビデオ表示用フレームバッファ参照ポインタ
   count = 1;

  // SPIの初期化・設定
  if (spino == 2) {
    pSPI = new SPIClass(2);
    _spi_dma    = MYSPI_DMA;
    _spi_dma_ch = MYSPI2_DMA_CH;
  } else {
    pSPI = &SPI;
    _spi_dma    = MYSPI_DMA;
    _spi_dma_ch = MYSPI1_DMA_CH;
  };
  pSPI->begin(); 
  pSPI->setBitOrder(MSBFIRST);  // データ並びは上位ビットが先頭
  pSPI->setDataMode(SPI_MODE3); // MODE3(MODE1でも可)
	if (_spino == 2) {
      pSPI->setClockDivider(screen_type[_screen].spiDiv-1); // クロックをシステムクロック36MHzの1/8に設定
	} else {
      pSPI->setClockDivider(screen_type[_screen].spiDiv);    // クロックをシステムクロック72MHzの1/16に設定
	}
	pSPI->dev()->regs->CR1 |=SPI_CR1_BIDIMODE_1_LINE|SPI_CR1_BIDIOE; // 送信のみ利用の設定

  // SPIデータ転送用DMA設定
  dma_init(_spi_dma);
  dma_attach_interrupt(_spi_dma, _spi_dma_ch, &DMA1_CH3_handle);
  spi_tx_dma_enable(pSPI->dev());  
  
  /// タイマ2の初期設定
  nvic_irq_set_priority(NVIC_TIMER2, IRQ_PRIORITY); // 割り込み優先レベル設定
  Timer2.pause();                             // タイマー停止
  Timer2.setPrescaleFactor(NTSC_TIMER_DIV);   // システムクロック 72MHzを24MHzに分周 
  Timer2.setOverflow(1524);                   // カウンタ値1524でオーバーフロー発生 63.5us周期

  // +4.7us 水平同期信号出力設定
  pinMode(PWM_CLK,PWM);          // 同期信号出力ピン(PWM)
  timer_cc_set_pol(TIMER2,2,1);  // 出力をアクティブLOWに設定
  pwmWrite(PWM_CLK, 112);        // パルス幅を4.7usに設定(仮設定)
  
  // +9.4us 映像出力用 割り込みハンドラ登録
  Timer2.setCompare(1, 225-60);  // オーバーヘッド分等の差し引き
  Timer2.setMode(1,TIMER_OUTPUTCOMPARE);
  Timer2.attachInterrupt(1, handle_vout);   

  Timer2.setCount(0);
  Timer2.refresh();       // タイマーの更新
  Timer2.resume();        // タイマースタート  
}

// NTSCビデオ表示終了
void TNTSC_class::end() {
  Timer2.pause();
  Timer2.detachInterrupt(1);
  spi_tx_dma_disable(pSPI->dev());  
  dma_detach_interrupt(_spi_dma, _spi_dma_ch);
  pSPI->end();
  if (!flgExtVram)
     free(vram);
  if (_spino == 2) {
  	delete pSPI;
  	//pSPI->~SPIClass();
  }	
}

// VRAMアドレス取得
uint8_t* TNTSC_class::VRAM() {
  return vram;  
}

// 画面クリア
void TNTSC_class::cls() {
  memset(vram, 0, _vram_size);
}

// フレーム間待ち
void TNTSC_class::delay_frame(uint16_t x) {
  while (x) {
    while (count != _ntscHeight + NTSC_VTOP);
    while (count == _ntscHeight + NTSC_VTOP);
    x--;
  }
}

	
TNTSC_class TNTSC;

