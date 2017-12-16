#define SIZE_LINE 128    // コマンドライン入力バッファサイズ + NULL
#define SIZE_IBUF 128    // 中間コード変換バッファサイズ

extern char lbuf[SIZE_LINE];
extern char tbuf[SIZE_LINE];
extern int32_t tbuf_pos;
extern unsigned char ibuf[SIZE_IBUF];

extern uint8_t err; // Error message index

uint8_t toktoi();
void putlist(unsigned char* ip, uint8_t devno=0);

// メモリ書き込みポインタのクリア
static inline void cleartbuf() {
  tbuf_pos=0;
  memset(tbuf,0,SIZE_LINE);
}
