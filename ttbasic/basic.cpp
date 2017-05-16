/*
 TOYOSHIKI Tiny BASIC for Arduino
 (C)2012 Tetsuya Suzuki
 GNU General Public License
   2017/03/22 Modified by Tamakichi、for Arduino STM32
 */

//
// 2017/03/22 修正, Arduino STM32、フルスクリーン対応, by たま吉さん
// 2017/03/25 修正, INKEY()関数の追加, 中断キーiを[ESC]から[CTRL-C]に変更
// 2017/03/26 修正, 文法において下記の変更・追加
// 1)変更: 命令文区切りを';'から':'に変更
// 2)追加: PRINTの行継続を';'でも可能とする
// 3)追加: IF文の不一致判定を"<>"でも可能とする
// 4)追加: 演算子 剰余計算 '%' の追加
// 5)追加: VPEEK() スクリーン位置の文字コード参照
// 6)追加: CHR$(),ASC()関数の追加
//
// 2017/03/29 修正, 文法において下記の変更・追加
// 1)追加: RENUMコマンドの追加
// 2)追加: 定数 HIGH、LOW,ピン番号、ピンモード設定の追加
// 2017/03/30 修正, 文法において下記の変更・追加
// 1)追加: GPIO, OUT,IN,ANAコマンドの追加
// 2)変更: SIZE() をFREE()に変更
// 3)変更: OKをエラーとしない対応
// 4)変更: プログラム中断を[ESC]でも可能とする、INKEY()の処理修正
// 5)追加: エラーメッセージ"Illegal value"の追加
//
// 2017/03/31 修正, 文法において下記の変更・追加
//  1)追加: HEX$()関数の追加
//  2)追加: 16進数表記対応: $+16進数文字(1桁～4桁)
//  3)修正: 文字列囲みにシングルクォーテーションを利用出来ないようにする
//  4)追加: コメント文をシングルクォーテーションでお可能とする
//  5)追加: GOTO,GOSUBをラベル対応(例: GOTO "ラベル" ダブルクォーテーション囲み)
//  7)追加: 配列の連続値設定を可能した(例: @(0)=1,2,3,4,5)
//  8)修正: 配列のインデックス指定に0以下を指定した場合のエラー処理追加
//  9)追加: SHIFTOUTコマンドの追加
// 10)追加: シフト演算子 ">>","<<"の追加(負号考慮なし)
// 11)追加: BIN$()の追加
// 12)追加: 算術積、算術和 '|'、'&'演算子の追加
// 13)追加: TICK() 起動から現在までの時間の取得
// 2017/04/1  修正, 文法において下記の変更・追加
//  1)修正: BIN$()の不具合修正
//  2)追加: PEEK,POKEの追加
// 2017/04/2  修正, 文法において下記の変更・追加
//  3)追加: 定数 MEM,VRAMの追加,作業用領域MEMの追加
//  4)追加: I2Cコマンド,I2CW(),I2CR()の追加
//  5)追加: SHIFTIN()コマンドの追加
//  6)追加: SAVE,LOADコマンドの追加
//  7)追加: SETDATE,GETDATE,GETTIME,DATEコマンドの追加
//  8)追加: PRINT文の桁指定 #で-数値に0埋め表示
// 2017/04/4  修正, 文法において下記の変更・追加
//  1)修正: LOAD,SAVEの引数エラーチェックの修正(省略or定数値のみ可能とする)
//  2)修正: RENUMの引数の有効性チェックを追加
//  3)追加: RESTTICKの追加
//  4)修正: コマンド入力ラインの右空白文字のトリム処理の追加(スペース打ち文法エラー対策)
//  5)追加: REDRAWの追加（コンソール画面再表示）
//  6)修正: LISTの第2引数で表示終了行の指定を可能にした
// 2017/04/5 修正, 文法において下記の変更・追加
//  1)追加: IF文のELSE対応
//  2)修正: LIST出力時のIF文の実行文前のスペースが詰まる不具合対応
// 2017/04/6 修正, 文法において下記の変更・追加
//  1)追加: EEPROM(エミュレーション対応) EEPFORMAT,EEPWRITE,EEPREADの追加
//  2)修正: EEPROM対応に伴う、プログラム保存領域のアドレス変更
//  3)追加：DELETEコマンドで指定行のプログラムを削除出来るようにしたい
//  4)修正: INPUT文でカーソルを表示,数値以外でBEEPを鳴らすように修正
// 2017/04/7 修正, 文法において下記の変更・追加
//  1)修正: INPUT文のオーバーフロー時発生時の既定値設定機能追加
//  2)修正: RNDの引数範囲チェック追加、仕様を0～指定値未満に変更
//  3)追加: 論理積AND、論理和ORの追加,NOTの追加、ビット反転~、XOR^の追加
//  4)修正: <>を!=に変更
// 2017/04/12 
//  1)修正: プログラム実行中断のメッセージの変更(Break in xx)
//  2)追加: Line too longのエラーメッセージ追加
// 2017/04/13
//  1)追加: PSET,LINE,RECT,CIRCLEの追加
//  2)追加: SOPEN,SCLOSE,SWRITE,SREAD,SREADY,SPRINTの追加
/// 2017/04/14 修正 ELSEの前は必ず空白にする設定
//  修正日 2017/04/17, BITMAPコマンドの追加
//  修正日 2017/04/18, SMODEコマンド(シリアルポート設定機能の追加)
//                     FILEコマンドの追加、プログラムサイズを4kバイトに修正
//                     プログラム保存最大数を10個に修正
//                     GSCROLL,CSCROLLコマンドの追加
//  修正日 2017/04/25, GSCROLLの機能変更、EXPORTコマンドの追加
//                     UP,DOWN,RIGHT,LEFTの定数追加
//                     GPEEK(),GINP(),GPRINTの追加
//  修正日 2017/04/26, RECT,CSCROLL,GSCROLLの仕様変更
//  修正日 2017/04/27, PWM対応(GPIOコマンド修正、POUTコマンド追加)の仕様変更
//  修正日 2017/04/28, CONFIGコマンドの追加(NTSC補正対応),SAVECONFIGの追加
//  修正日 2017/04/29, MAP(),DMP$()の追加（スケール変換、小数表示補助),LRUNの追加
//  修正日 2017/05/02, MAP(),SAVECONFIGの不具合対応
//  修正日 2017/05/03, 文字コード(32～126,128～255)がシリアル経由で入力可能に修正
//  修正日 2017/05/10, RECTの不具合対応
//  修正日 2017/05/13, エラーメッセージ出力の不具合対応
//  修正日 2017/05/14, SHIFTIN()の引数追加(CD4021対応)
//  修正日 2017/05/16, POKEの不具合修正,PRINTでのエラー時のメッセージ出力処理の修正
//
// Depending on device functions
// TO-DO Rewrite these functions to fit your machine
#define STR_EDITION "Arduino STM32"
#define STR_VARSION "Edition V0.81"

// I2Cライブラリの選択
#define I2C_USE_HWIRE  1     // 1:HWire 0:Wire(ソフトエミュレーション)

// 内蔵RTCの利用指定
#define USE_INNERRTC   1     // 0:利用しない 1:利用する

// SRAMの物理サイズ(バイト)
#define SRAM_SIZE      20480 // STM32F103C8T6

#include <Arduino.h>
#include <stdlib.h>
#include <wirish.h>
// **** スクリーン管理 *************
#include "tscreen.h"
// スクリーン制御
tscreen sc; 
#define KEY_ENTER 13

// **** I2Cライブラリの利用設定 ****
#if I2C_USE_HWIRE == 0
  #include <Wire.h>
  #define I2C_WIRE  Wire
#else
  #include <HardWire.h>
  HardWire HWire(1, I2C_FAST_MODE); // I2C1を利用
  #define I2C_WIRE  HWire
#endif

// **** プロフラム保存用定義 ********
#include <TFlash.h>
#define FLASH_PAGE_SIZE        1024
#define FLASH_START_ADDRESS    ((uint32_t)(0x8000000))
#define FLASH_PAGE_NUM         128     // 全ページ数
#define FLASH_PRG_START_PAGE   (FLASH_PAGE_NUM-FLASH_PAGE_PAR_PRG*FLASH_SAVE_NUM)  // 利用開始ページ
#define FLASH_PAGE_PAR_PRG     4       // 1プログラム当たりの利用ページ数
#define FLASH_SAVE_NUM         10      // 保存可能数

// **** EEPROMエミュレーション ******
#include <EEPROM.h>
extern EEPROMClass EEPROM;
#define EEPROM_PAGE0 (((uint32_t)(0x8000000))+FLASH_PAGE_SIZE*(FLASH_PAGE_NUM-2))
#define EEPROM_PAGE1 (((uint32_t)(0x8000000))+FLASH_PAGE_SIZE*(FLASH_PAGE_NUM-1))

// *** システム設定関連 **************
#define CONFIG_NTSC 65534  // EEPROM NTSC設定値保存番号
#define CONFIG_KBD  65533  // EEPROM キーボード設定
#define CONFIG_PRG  65532  // 自動起動設定

typedef struct {
  int16_t NTSC;        // NTSC設定 (0,1,2,3)
  int16_t KEYBOARD;    // キーボード設定 (0:JP, 1:US)
  int16_t STARTPRG;    // 自動起動(-1,なし 0～9:保存プログラム番号)
} SystemConfig;
SystemConfig CONFIG;

uint8_t loadConfig();
uint8_t saveConfig();

// **** RTC用宣言 ********************
#if USE_INNERRTC == 1
  #include <RTClock.h>
  #include <time.h>
  RTClock rtc(RTCSEL_LSE);
#endif

// **** PWM用設定 ********************
#if F_CPU == 72000000L
#define TIMER_DIV 72
#else if  F_CPU == 48000000L
#define TIMER_DIV 48
#endif

// TOYOSHIKI TinyBASIC symbols
// TO-DO Rewrite defined values to fit your machine as needed
#define SIZE_LINE 255    // コマンドライン入力バッファサイズ + NULL
#define SIZE_IBUF 255    // 中間コード変換バッファサイズ
#define SIZE_LIST 4096   // プログラム領域サイズ(4kバイト)
#define SIZE_ARRY 100    // 配列変数サイズ(@(0)～@(99)
#define SIZE_GSTK 6      // GOSUB stack size(2/nest)
#define SIZE_LSTK 15     // FOR stack size(5/nest)
#define SIZE_MEM  2048   // 自由利用データ領域

void tv_write(uint8_t c) ;
//void mprint(uint8_t x, uint8_t y, char* str);

// Terminal control(文字の表示・入力は下記の3関数のみ利用)
#define c_getch( ) sc.get_ch()
#define c_kbhit( ) sc.isKeyIn()

// 文字の出力
inline void c_putch(uint8_t c, uint8_t devno = 0) {
  if (devno == 0)
    sc.putch(c);
  else if (devno == 1)
    sc.Serial_write(c);
  else if (devno == 2)
    sc.gputch(c);  
} 

// 定数
#define CONST_HIGH   1
#define CONST_LOW    0
#define CONST_LSB    LSBFIRST
#define CONST_MSB    MSBFIRST
#define SRAM_TOP     0x20000000

// 追加コマンドの宣言
void icls();
void iwait(uint16_t tm);
void ilocate();
void icolor();
void iattr();
int16_t iinkey();
int16_t ivpeek();
void igpio();
void idwrite();
void ihex(uint8_t devno);
void ibin(uint8_t devno);
void idmp(uint8_t devno);
void idec();
void ishiftOut();
int16_t ishiftIn();
void ipoke();
int16_t ii2cw();
int16_t ii2cr();
void isetDate();
void igetDate();
void igetTime();
void idate();
void isave();
void iload();
void iconfig();
void isaveconfig();

void ieepformat();
void ieepwrite();
int16_t ieepread(uint16_t addr);

void ipset();
void iline();
void icircle();
void irect();
void ibitmap();
void icscroll();
void igscroll();

void iswrite();
void isopen();
void isclose();
void ismode();

void itone();
void inotone();
void iexport();

int16_t igpeek();
int16_t iginp();
void igprint();

void ipwm();
int16_t imap();
void idelete();
uint8_t ilrun();

// tick用支援関数
void resetTick() {
  systick_uptime_millis = 0;
}

void newline(uint8_t devno=0) {
 if (devno==0)
   sc.newLine();
  else if (devno == 1)
    sc.Serial_newLine();
  else if (devno == 2)
    sc.gputch('\n');
}

// Return random number
short getrnd(short value) {
  return random(value);
}

// Prototypes (necessity minimum)
short iexp(void);

// ピンモードの設定
const WiringPinMode pinType[] = {
  OUTPUT_OPEN_DRAIN, OUTPUT, INPUT_PULLUP, INPUT_PULLDOWN, INPUT_ANALOG, INPUT, PWM,
};

// Keyword table
const char *kwtbl[] = {
  "GOTO", "GOSUB", "RETURN","TONE", "NOTONE",
  "FOR", "TO", "STEP", "NEXT",
  "IF", "REM", "END", "ELSE",
  "PRINT", "LET", 
  "CLS", "WAIT", "LOCATE", "COLOR", "ATTR" ,"INKEY", "?", "VPEEK", "CHR$" , "ASC", "RENUM", "HEX$", "BIN$",
  "HIGH", "LOW", 
  ",", ";", ":", "\'",
  "-", "+", "*", "/", "%", "(", ")", "$", "<<", ">>", "|", "&", 
  ">=", "#", ">", "=", "<=", "!=", "<", "AND", "OR", "!", "~", "^",
  "@", "RND", "ABS", "FREE", "TICK", "POKE", "PEEK", "I2CW", "I2CR",
  "SETDATE", "GETDATE", "GETTIME", "DATE", "CONFIG", "SAVECONFIG", "DELETE", "LRUN",
  "OUTPUT_OD", "OUTPUT", "INPUT_PU", "INPUT_PD", "ANALOG", "INPUT_FL", "PWM",
  "INPUT", "GPIO", "OUT", "IN", "ANA", "SHIFTOUT", "SHIFTIN", "POUT","MAP", "DMP$",
  "PA00", "PA01", "PA02", "PA03", "PA04", "PA05", "PA06", "PA07", "PA08", 
  "PA09", "PA10", "PA11", "PA12", "PA13","PA14","PA15",
  "PB00", "PB01", "PB02", "PB03", "PB04", "PB05", "PB06", "PB07", "PB08", 
  "PB09", "PB10", "PB11", "PB12", "PB13","PB14","PB15",
  "PC13", "PC14","PC15", "CW", "CH","GW","GH",
  "LSB", "MSB", "MEM", "VRAM", "VAR", "ARRAY",
  "LIST", "RUN", "NEW", "OK", "SAVE", "LOAD", "FILES","RESETTICK", "REDRAW",
  "EEPFORMAT", "EEPWRITE", "EEPREAD",
  "PSET","LINE","RECT","CIRCLE", "BITMAP", "CSCROLL", "GSCROLL","GPEEK","GINP", "GPRINT",
  "SWRITE", "SREADY", "SREAD", "SPRINT", "SOPEN", "SCLOSE", "SMODE", "EXPORT",
  "UP","DOWN","RIGHT","LEFT",
};

// Keyword count
#define SIZE_KWTBL (sizeof(kwtbl) / sizeof(const char*))

// i-code(Intermediate code) assignment
enum {
  I_GOTO, I_GOSUB, I_RETURN, I_TONE, I_NOTONE,
  I_FOR, I_TO, I_STEP, I_NEXT, 
  I_IF, I_REM, I_END, I_ELSE, 
  I_PRINT, I_LET,  
  I_CLS, I_WAIT, I_LOCATE, I_COLOR, I_ATTR, I_INKEY, I_QUEST, I_VPEEK, I_CHR, I_ASC, I_RENUM, I_HEX, I_BIN,
  I_HIGH, I_LOW,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT,
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_DOLLAR, I_LSHIFT, I_RSHIFT, I_OR, I_AND,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_LT, I_LAND, I_LOR, I_LNOT, I_BITREV, I_XOR,
  I_ARRAY, I_RND, I_ABS, I_FREE, I_TICK, I_POKE, I_PEEK, I_ISND, I_IRCV,
 
  I_SETDATE, I_GETDATE, I_GETTIME, I_DATE, I_CONFIG, I_SAVECONFIG, I_DELETE, I_LRUN,
  I_OUTPUT_OPEN_DRAIN, I_OUTPUT, I_INPUT_PULLUP, I_INPUT_PULLDOWN, I_INPUT_ANALOG, I_INPUT_F,  I_PWM,
  
  I_INPUT, I_GPIO, I_DOUT, I_DIN, I_ANA, I_SHIFTOUT, I_SHIFTIN, I_POUT, I_MAP, I_DMP,
  
  I_PA0, I_PA1, I_PA2, I_PA3, I_PA4, I_PA5, I_PA6, I_PA7, I_PA8, 
  I_PA9, I_PA10, I_PA11, I_PA12, I_PA13,I_PA14,I_PA15,
  I_PB0, I_PB1, I_PB2, I_PB3, I_PB4, I_PB5, I_PB6, I_PB7, I_PB8, 
  I_PB9, I_PB10, I_PB11, I_PB12, I_PB13,I_PB14,I_PB15,
  I_PC13, I_PC14,I_PC15,I_CW, I_CH, I_GW, I_GH,
  I_LSB, I_MSB, I_MEM, I_VRAM, I_MVAR, I_MARRAY,
  I_LIST, I_RUN, I_NEW, I_OK, I_SAVE, I_LOAD, I_FILES, I_RESETTICK, I_REFLESH,
  I_EEPFORMAT, I_EEPWRITE, I_EEPREAD,
  I_PSET, I_LINE, I_RECT, I_CIRCLE, I_BITMAP, I_CSCROLL, I_GSCROLL, I_GPEEK, I_GINP, I_GPRINT,
  I_SWRITE, I_SREADY, I_SREAD, I_SPRINT, I_SOPEN, I_SCLOSE, I_SMODE, I_EXPORT,
  I_UP, I_DOWN, I_RIGHT, I_LEFT,
  I_NUM, I_VAR, I_STR, I_HEXNUM, 
  I_EOL
};

// List formatting condition
// 後ろに空白を入れない中間コード
const unsigned char i_nsa[] = {
  I_RETURN, I_END, 
  I_CLS,
  I_HIGH, I_LOW, I_CW, I_CH, I_GW, I_GH, 
  I_UP, I_DOWN, I_RIGHT, I_LEFT,
  I_INKEY,I_VPEEK, I_CHR, I_ASC, I_HEX, I_BIN,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT,I_QUEST,
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_DOLLAR, I_LSHIFT, I_RSHIFT, I_OR, I_AND,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_LT, I_LNOT, I_BITREV, I_XOR,
  I_ARRAY, I_RND, I_ABS, I_FREE, I_TICK, I_PEEK, I_ISND, I_IRCV,
  I_OUTPUT_OPEN_DRAIN, I_OUTPUT, I_INPUT_PULLUP, I_INPUT_PULLDOWN, I_INPUT_ANALOG, I_INPUT_F, I_PWM,
  I_DIN, I_ANA, I_SHIFTIN, I_MAP, I_DMP,
  I_PA0, I_PA1, I_PA2, I_PA3, I_PA4, I_PA5, I_PA6, I_PA7, I_PA8, 
  I_PA9, I_PA10, I_PA11, I_PA12, I_PA13,I_PA14,I_PA15,
  I_PB0, I_PB1, I_PB2, I_PB3, I_PB4, I_PB5, I_PB6, I_PB7, I_PB8, 
  I_PB9, I_PB10, I_PB11, I_PB12, I_PB13,I_PB14,I_PB15,
  I_PC13, I_PC14,I_PC15,
  I_LSB, I_MSB, I_MEM, I_VRAM, I_MVAR, I_MARRAY, I_EEPREAD,
  I_SREAD, I_SREADY, I_GPEEK, I_GINP,
};

// 前が定数か変数のとき前の空白をなくす中間コード
const unsigned char i_nsb[] = {
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_LSHIFT, I_RSHIFT, I_OR, I_AND,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_LT, I_LNOT, I_BITREV, I_XOR,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT, I_EOL
};

// 必ず前に空白を入れる中間コード
const unsigned char i_sf[] = {
  I_ATTR, I_CLS, I_COLOR, I_DATE, I_END, I_FILES, I_TO, I_STEP,I_QUEST,
  I_GETDATE,I_GETTIME,I_GOSUB,I_GOTO,I_GPIO,I_INKEY,I_INPUT,I_LET,I_LIST,I_ELSE,
  I_LOAD,I_LOCATE,I_NEW,I_DOUT,I_POKE,I_PRINT,I_REFLESH,I_REM,I_RENUM,I_RESETTICK,
  I_RETURN,I_RUN,I_SAVE,I_SETDATE,I_SHIFTOUT,I_WAIT,I_EEPFORMAT, I_EEPWRITE, 
  I_PSET, I_LINE, I_RECT, I_CIRCLE, I_BITMAP, I_SWRITE, I_SPRINT,  I_SOPEN, I_SCLOSE,I_SMODE,
  I_TONE, I_NOTONE, I_CSCROLL, I_GSCROLL,I_EXPORT
};

const uint8_t pwmpins[] = {PA8,PA9,PA10,PB1,PB0};

// exception search function
char sstyle(unsigned char code,
  const unsigned char *table, unsigned char count) {
  while(count--) //中間コードの数だけ繰り返す
    if (code == table[count]) //もし該当の中間コードがあったら
      return 1; //1を持ち帰る
  return 0; //（なければ）0を持ち帰る
}

// exception search macro
#define nospacea(c) sstyle(c, i_nsa, sizeof(i_nsa))
#define nospaceb(c) sstyle(c, i_nsb, sizeof(i_nsb))
#define spacef(c) sstyle(c, i_sf, sizeof(i_sf))

// Error messages
unsigned char err;// Error message index
const char* errmsg[] = {
  "OK",
  "Devision by zero",
  "Overflow",
  "Subscript out of range",
  "Icode buffer full",
  "List full",
  "GOSUB too many nested",
  "RETURN stack underflow",
  "FOR too many nested",
  "NEXT without FOR",
  "NEXT without counter",
  "NEXT mismatch FOR",
  "FOR without variable",
  "FOR without TO",
  "LET without variable",
  "IF without condition",
  "Undefined line number or label",
  "\'(\' or \')\' expected",
  "\'=\' expected",
  "Illegal command",
  "Illegal value",      // 追加
  "Out of range value", // 追加
  "Program not found",  // 追加
  "Syntax error",
  "Internal error",
  "Break",
  "Line too long",
  "EEPROM out size",     // 追加
  "EEPROM bad address",  // 追加
  "EEPROM bad FLASH",    // 追加
  "EEPROM not INIT",     // 追加
  "EEPROM not vaid page",// 追加
};

// Error code assignment
enum {
  ERR_OK,
  ERR_DIVBY0,
  ERR_VOF,
  ERR_SOR,
  ERR_IBUFOF, ERR_LBUFOF,
  ERR_GSTKOF, ERR_GSTKUF,
  ERR_LSTKOF, ERR_LSTKUF,
  ERR_NEXTWOV, ERR_NEXTUM, ERR_FORWOV, ERR_FORWOTO,
  ERR_LETWOV, ERR_IFWOC,
  ERR_ULN,
  ERR_PAREN, ERR_VWOEQ,
  ERR_COM,
  ERR_VALUE, // 追加
  ERR_RANGE, // 追加
  ERR_NOPRG, // 追加
  ERR_SYNTAX,
  ERR_SYS,
  ERR_CTR_C,
  ERR_LONG,                 // 追加
  ERR_EEPROM_OUT_SIZE,      // 追加
  ERR_EEPROM_BAD_ADDRESS,   // 追加
  ERR_EEPROM_BAD_FLASH,     // 追加
  ERR_EEPROM_NOT_INIT,      // 追加
  ERR_EEPROM_NO_VALID_PAGE, // 追加
};

// RAM mapping
char lbuf[SIZE_LINE];             // コマンド入力バッファ
unsigned char ibuf[SIZE_IBUF];    // i-code conversion buffer
short var[26];                    // 変数領域
short arr[SIZE_ARRY];             // 配列領域
unsigned char listbuf[SIZE_LIST]; // プログラムリスト領域
uint8_t mem[SIZE_MEM];            // 自由利用データ領域

unsigned char* clp;               // Pointer current line
unsigned char* cip;               // Pointer current Intermediate code
unsigned char* gstk[SIZE_GSTK];   // GOSUB stack
unsigned char gstki;              // GOSUB stack index
unsigned char* lstk[SIZE_LSTK];   // FOR stack
unsigned char lstki;              // FOR stack index

uint8_t prevPressKey = 0;         // 直前入力キーの値(INKEY()、[ESC]中断キー競合防止用)
uint8_t lfgSerial1Opened = false;  // Serial1のオープン設定フラグ

// Standard C libraly (about) same functions
char c_toupper(char c) {
  return(c <= 'z' && c >= 'a' ? c - 32 : c);
}
char c_isprint(char c) {
  //return(c >= 32 && c <= 126);
  return(c >= 32 && c!=127 );
}
char c_isspace(char c) {
  return(c == ' ' || (c <= 13 && c >= 9));
}
char c_isdigit(char c) {
  return(c <= '9' && c >= '0');
}
char c_isalpha(char c) {
  return ((c <= 'z' && c >= 'a') || (c <= 'Z' && c >= 'A'));
}

// 16進数文字チェック
char c_ishex(char c) {
  return( (c <= '9' && c >= '0') || (c <= 'f' && c >= 'a') || (c <= 'F' && c >= 'A') );
}

// 文字列の右側の空白文字を削除する
char* tlimR(char* str) {
  uint16_t len = strlen(str);
  for (uint16_t i = len - 1; i>0 ; i--) {
    if (str[i] == ' ') {
      str[i] = 0;
    } else {
      break;
    }
  }
  return str;
}

// コマンド引数取得
inline uint8_t getParam(int16_t& prm, int16_t  v_min,  int16_t  v_max, uint8_t flgCmma) {
  prm = iexp(); 
  if (!err &&  (prm < v_min || prm > v_max)) 
    err = ERR_VALUE;
  else if (flgCmma && *cip++ != I_COMMA) {
    err = ERR_SYNTAX;
 }
  return err;
}
inline uint8_t getParam(uint16_t& prm, uint8_t flgCmma) {
  prm = iexp(); 
  if (!err && flgCmma && *cip++ != I_COMMA) {
   err = ERR_SYNTAX;
  }
  return err;
}
inline uint8_t getParam(int16_t& prm, uint8_t flgCmma) {
  prm = iexp(); 
  if (!err && flgCmma && *cip++ != I_COMMA) {
   err = ERR_SYNTAX;
  }
  return err;
}
inline uint8_t getParam(int32_t& prm, uint8_t flgCmma) {
  prm = iexp(); 
  if (!err && flgCmma && *cip++ != I_COMMA) {
   err = ERR_SYNTAX;
  }
  return err;
}

inline uint8_t checkOpen() {
  if (*cip != I_OPEN)  err = ERR_PAREN;
  else cip++;
  return err;
}
inline uint8_t checkClose() {
  if (*cip != I_CLOSE)  err = ERR_PAREN;
  else cip++;
  return err;
}

// 1桁16進数文字を整数に変換する
uint16_t hex2value(char c) {
  if (c <= '9' && c >= '0')
    return c - '0';
  else if (c <= 'f' && c >= 'a')
    return c - 'a' + 10;
  else if (c <= 'F' && c >= 'A')
    return c - 'A' + 10;
  return 0;
}

void c_puts(const char *s, uint8_t devno=0) {
  while (*s) c_putch(*s++, devno); //終端でなければ出力して繰り返す
}

// Print numeric specified columns
// 引数
//  value : 出力対象数値
//  d     : 桁指定(0で指定無し)
// 機能
// 'SNNNNN' S:符号 N:数値 or 空白 
//  dで桁指定時は空白補完する
//
void putnum(short value, short d, uint8_t devno=0) {
  unsigned char dig;  // 桁位置
  unsigned char sign; // 負号の有無（値を絶対値に変換した印）
  char c = ' ';
  if (d < 0) {
    d = -d;
    c = '0';
  }

  if (value < 0) {     // もし値が0未満なら
    sign = 1;          // 負号あり
    value = -value;    // 値を絶対値に変換
  } else {
    sign = 0;          // 負号なし
  }

  lbuf[6] = 0;         // 終端を置く
  dig = 6;             // 桁位置の初期値を末尾に設定
  do { //次の処理をやってみる
    lbuf[--dig] = (value % 10) + '0'; // 1の位を文字に変換して保存
    value /= 10;                      // 1桁落とす
  } while (value > 0);                // 値が0でなければ繰り返す

  if (sign) //もし負号ありなら
    lbuf[--dig] = '-'; // 負号を保存

  while (6 - dig < d) { // 指定の桁数を下回っていれば繰り返す
    c_putch(c,devno);   // 桁の不足を空白で埋める
    d--;                // 指定の桁数を1減らす
  }
  c_puts(&lbuf[dig],devno);   // 桁位置からバッファの文字列を表示
}

// 16進数の出力
// 引数
//  value : 出力対象数値
//  d     : 桁指定(0で指定無し)
// 機能
// 'XXXX' X:数値
//  dで桁指定時は0補完する
//  符号は考慮しない
// 
void putHexnum(short value, uint8_t d, uint8_t devno=0) {
  uint16_t  hex = (uint16_t)value; // 符号なし16進数として参照利用する
  uint16_t  h;
  uint16_t dig;

  // 表示に必要な桁数を求める
  if (hex >= 0x1000) 
    dig = 4;
  else if (hex >= 0x100) 
    dig = 3;
  else if (hex >= 0x10) 
    dig = 2;
  else 
    dig = 1;

  if (d != 0 && d > dig) 
    dig = d;

  for (uint8_t i = 0; i < 4; i++) {
    h = ( hex >> (12 - i * 4) ) & 0x0f;
    lbuf[i] = (h >= 0 && h <= 9) ? h + '0': h + 'A' - 10;
  }
  lbuf[4] = 0;
  c_puts(&lbuf[4-dig],devno);
}

// 2進数の出力
// 引数
//  value : 出力対象数値
//  d     : 桁指定(0で指定無し)
// 機能
// 'BBBBBBBBBBBBBBBB' B:数値
//  dで桁指定時は0補完する
//  符号は考慮しない
// 
void putBinnum(short value, uint8_t d, uint8_t devno=0) {
  uint16_t  bin = (uint16_t)value; // 符号なし16進数として参照利用する
  uint16_t  b;
  uint16_t  dig = 0;
  
  for (uint8_t i = 0; i < 16; i++) {
    b =(bin>>(15-i)) & 1;
    lbuf[i] = b ? '1':'0';
    if (!dig && b) 
      dig = 16-i;
  }
  lbuf[16] = 0;

  if (d > dig)
    dig = d;
  c_puts(&lbuf[16-dig],devno);
}

// Input numeric and return value
// Called by only INPUT statement
short getnum() {
  short value, tmp; //値と計算過程の値
  char c; //文字
  unsigned char len; //文字数
  unsigned char sign; //負号

  len = 0; //文字数をクリア
  while ((c = c_getch()) != KEY_ENTER) { //改行でなければ繰り返す
    //［BackSpace］キーが押された場合の処理（行頭ではないこと）
    if (((c == 8) || (c == 127)) && (len > 0)) {
      len--; //文字数を1減らす
      //c_putch(8); c_putch(' '); c_putch(8); //文字を消す
      sc.movePosPrevChar();
      sc.delete_char();
    } else
    //行頭の符号および数字が入力された場合の処理（符号込みで6桁を超えないこと）
    if ((len == 0 && (c == '+' || c == '-')) ||
      (len < 6 && c_isdigit(c))) {
      lbuf[len++] = c; //バッファへ入れて文字数を1増やす
      c_putch(c); //表示
    } else {
      sc.beep();
    }
  }
  newline(); //改行
  lbuf[len] = 0; //終端を置く

  switch (lbuf[0]) { //先頭の文字で分岐
  case '-': //「-」の場合
    sign = 1; //負の値
    len = 1;  //数字列はlbuf[1]以降
    break;
  case '+': //「+」の場合
    sign = 0; //正の値
    len = 1;  //数字列はlbuf[1]以降
    break;
  default:  //どれにも該当しない場合
    sign = 0; //正の値
    len = 0;  //数字列はlbuf[0]以降
    break;
  }

  value = 0; //値をクリア
  tmp = 0; //計算過程の値をクリア
  while (lbuf[len]) { //終端でなければ繰り返す
    tmp = 10 * value + lbuf[len++] - '0'; //数字を値に変換
    if (value > tmp) { //もし計算過程の値が前回より小さければ
      err = ERR_VOF; //オーバーフローを記録
    }
    value = tmp; //計算過程の値を記録
  }

  if (sign) //もし負の値なら
    return -value; //負の値に変換して持ち帰る

  return value; //値を持ち帰る
}

// Convert token to i-code
// Return byte length or 0
unsigned char toktoi() {
  unsigned char i;        // ループカウンタ（一部の処理で中間コードに相当）
  unsigned char len = 0;  // 中間コードの並びの長さ
  char* pkw = 0;          // ひとつのキーワードの内部を指すポインタ
  char* ptok;             // ひとつの単語の内部を指すポインタ
  char* s = lbuf;         // 文字列バッファの内部を指すポインタ
  char c;                 // 文字列の括りに使われている文字（「"」または「'」）
  short value;            // 定数
  short tmp;              // 変換過程の定数
  uint16_t hex;           // 16進数定数
  uint16_t hcnt;          // 16進数桁数

  while (*s) {                  //文字列1行分の終端まで繰り返す
    while (c_isspace(*s)) s++;  //空白を読み飛ばす

    //キーワードテーブルで変換を試みる
    for (i = 0; i < SIZE_KWTBL; i++) {   // 全部のキーワードを試す
      pkw = (char *)kwtbl[i];            // キーワードの先頭を指す
      ptok = s;                          // 単語の先頭を指す

      //キーワードと単語の比較
      while (                            // 次の条件が成立する限り繰り返す
      (*pkw != 0) &&                     // キーワードの末尾に達していなくて
      (*pkw == c_toupper(*ptok))) {      // 文字が一致している
        pkw++;                           // キーワードの次の文字へ進む
        ptok++;                          // 単語の次の文字へ進む
      }

      //キーワードと単語が一致した場合の処理
      if (*pkw == 0) {                   // もしキーワードの末尾に達していたら（変換成功）
        if (len >= SIZE_IBUF - 1) {      // もし中間コードが長すぎたら
          err = ERR_IBUFOF;              // エラー番号をセット
          return 0;                      // 0を持ち帰る
        }

        ibuf[len++] = i;                 // 中間コードを記録
        s = ptok;                        // 文字列の処理ずみの部分を詰める
        break;                           // 単語→中間コード1個分の変換を完了
      }                                  // キーワードと単語が一致した場合の処理の末尾

    } //キーワードテーブルで変換を試みるの末尾

    // 16進数の変換を試みる $XXXX
    if (i == I_DOLLAR) {
      if (c_ishex(*ptok)) {   // もし文字が16進数文字なら
        hex = 0;              // 定数をクリア
        hcnt = 0;             // 桁数
        do { //次の処理をやってみる          
          hex = (hex<<4) + hex2value(*ptok++); // 数字を値に変換
          hcnt++;
        } while (c_ishex(*ptok)); //16進数文字がある限り繰り返す

        if (hcnt > 4) {      // 桁溢れチェック
          err = ERR_VOF;     // エラー番号オバーフローをセット
          return 0;          // 0を持ち帰る
        }
  
        if (len >= SIZE_IBUF - 3) { // もし中間コードが長すぎたら
          err = ERR_IBUFOF;         // エラー番号をセット
          return 0;                 // 0を持ち帰る
        }
        s = ptok; // 文字列の処理ずみの部分を詰める
        len--;    // I_DALLARを置き換えるために格納位置を移動
        ibuf[len++] = I_HEXNUM;  //中間コードを記録
        ibuf[len++] = hex & 255; //定数の下位バイトを記録
        ibuf[len++] = hex >> 8;  //定数の上位バイトを記録
      }      
    }

    //コメントへの変換を試みる
    if(i == I_REM|| i == I_SQUOT) {       // もし中間コードがI_REMなら
      while (c_isspace(*s)) s++;         // 空白を読み飛ばす
      ptok = s;                          // コメントの先頭を指す

      for (i = 0; *ptok++; i++);         // コメントの文字数を得る
      if (len >= SIZE_IBUF - 2 - i) {    // もし中間コードが長すぎたら
        err = ERR_IBUFOF;                // エラー番号をセット
        return 0;                        // 0を持ち帰る
      }

      ibuf[len++] = i;                   // コメントの文字数を記録
      while (i--) {                      // コメントの文字数だけ繰り返す
        ibuf[len++] = *s++;              // コメントを記録
      }
      break;                             // 文字列の処理を打ち切る（終端の処理へ進む）
    }

    if (*pkw == 0)                       // もしすでにキーワードで変換に成功していたら
      continue;                          // 繰り返しの先頭へ戻って次の単語を変換する

    ptok = s;                            // 単語の先頭を指す

    //定数への変換を試みる
    if (c_isdigit(*ptok)) {              // もし文字が数字なら
      value = 0;                         // 定数をクリア
      tmp = 0;                           // 変換過程の定数をクリア
      do { //次の処理をやってみる
        tmp = 10 * value + *ptok++ - '0'; // 数字を値に変換
        if (value > tmp) {                // もし前回の値より小さければ
          err = ERR_VOF;                  // エラー番号をセット
          return 0;                       // 0を持ち帰る
        }
        value = tmp; //0を持ち帰る
      } while (c_isdigit(*ptok)); //文字が数字である限り繰り返す

      if (len >= SIZE_IBUF - 3) { //もし中間コードが長すぎたら
        err = ERR_IBUFOF; //エラー番号をセット
        return 0; //0を持ち帰る
      }
      s = ptok; //文字列の処理ずみの部分を詰める
      ibuf[len++] = I_NUM; //中間コードを記録
      ibuf[len++] = value & 255; //定数の下位バイトを記録
      ibuf[len++] = value >> 8; //定数の上位バイトを記録
    }
    else

    //文字列への変換を試みる
    if (*s == '\"' ) { //もし文字が '\"'
      c = *s++; //「"」か「'」を記憶して次の文字へ進む
      ptok = s; //文字列の先頭を指す
      //文字列の文字数を得る
      for (i = 0; (*ptok != c) && c_isprint(*ptok); i++)
        ptok++;
      if (len >= SIZE_IBUF - 1 - i) { //もし中間コードが長すぎたら
        err = ERR_IBUFOF; //エラー番号をセット
        return 0; //0を持ち帰る
      }
      ibuf[len++] = I_STR; //中間コードを記録
      ibuf[len++] = i; //文字列の文字数を記録
      while (i--) { //文字列の文字数だけ繰り返す
        ibuf[len++] = *s++; //文字列を記録
      }
      if (*s == c) s++; //もし文字が「"」か「'」なら次の文字へ進む
    }
    else

    //変数への変換を試みる
    if (c_isalpha(*ptok)) { //もし文字がアルファベットなら
      if (len >= SIZE_IBUF - 2) { //もし中間コードが長すぎたら
        err = ERR_IBUFOF; //エラー番号をセット
        return 0; //0を持ち帰る
      }
      //もし変数が3個並んだら
      if (len >= 4 && ibuf[len - 2] == I_VAR && ibuf[len - 4] == I_VAR) {
        err = ERR_SYNTAX; //エラー番号をセット
        return 0; //0を持ち帰る
      }

      ibuf[len++] = I_VAR; //中間コードを記録
      ibuf[len++] = c_toupper(*ptok) - 'A'; //変数番号を記録
      s++; //次の文字へ進む
    }
    else

    //どれにも当てはまらなかった場合
    {
      err = ERR_SYNTAX; //エラー番号をセット
      return 0; //0を持ち帰る
    }
  } //文字列1行分の終端まで繰り返すの末尾

  ibuf[len++] = I_EOL; //文字列1行分の終端を記録
  return len; //中間コードの長さを持ち帰る
}

// Return free memory size
short getsize() {
  unsigned char* lp; //ポインタ

  for (lp = listbuf; *lp; lp += *lp); //ポインタをリストの末尾へ移動
  return listbuf + SIZE_LIST - lp - 1; //残りを計算して持ち帰る
}

// Get line numbere by line pointer
short getlineno(unsigned char *lp) {
  if(*lp == 0) //もし末尾だったら
    return 32767; //行番号の最大値を持ち帰る
  return *(lp + 1) | *(lp + 2) << 8; //行番号を持ち帰る
}

// Search line by line number
unsigned char* getlp(short lineno) {
  unsigned char *lp; //ポインタ

  for (lp = listbuf; *lp; lp += *lp) //先頭から末尾まで繰り返す
    if (getlineno(lp) >= lineno) //もし指定の行番号以上なら
      break; //繰り返しを打ち切る

  return lp; //ポインタを持ち帰る
}

// ラベルでラインポインタを取得する
// pLabelは [I_STR][長さ][ラベル名] であること
unsigned char* getlpByLabel(uint8_t* pLabel) {
  unsigned char *lp; //ポインタ
  uint8_t len;
  pLabel++;
  len = *pLabel; // 長さ取得
  pLabel++;      // ラベル格納位置
  
  for (lp = listbuf; *lp; lp += *lp)  { //先頭から末尾まで繰り返す
    if ( *(lp+3) == I_STR ) {
       if (len == *(lp+4)) {
           if (strncmp((char*)pLabel, (char*)(lp+5), len) == 0) {
              return lp;
           }
       }
    }  
  }
  return NULL;
}

// 行番号から行インデックスを取得する
uint16_t getlineIndex(uint16_t lineno) {
  unsigned char *lp; //ポインタ
  uint16_t index = 0;	
  uint16_t rc = 32767;
  for (lp = listbuf; *lp; lp += *lp) { // 先頭から末尾まで繰り返す
  	if (getlineno(lp) >= lineno) {     // もし指定の行番号以上なら
        rc = index;
  		break;                         // 繰り返しを打ち切る
  	}
  	index++;
  }
  return rc; 
}	

// ELSE中間コードをサーチする
// 引数   : 中間コードプログラムポインタ
// 戻り値 : NULL 見つからない
//          NULL以外 LESEの次のポインタ
//
uint8_t* getELSEptr(uint8_t* p) {
 uint8_t* rc = NULL;
 uint8_t* lp;
 
 // ブログラム中のGOTOの飛び先行番号を付け直す
  for (lp = p; *lp != I_EOL ; ) {
    switch(*lp) {
    case I_IF:    // IF命令
      goto DONE;
        break;
    case I_ELSE:  // ELSE命令
      rc = lp+1;
      goto DONE;
        break;
      break;
    case I_STR:     // 文字列
      lp += lp[1]+1;            
      break;
    case I_NUM:     // 定数
    case I_HEXNUM: 
      lp+=3;        // 整数2バイト+中間コード1バイト分移動
      break;
    case I_VAR:     // 変数
      lp+=2;        // 変数名
      break;
    default:        // その他
      lp++;
      break;
    }
  }  
DONE:
  return rc;
}


// プログラム行数を取得する
uint16_t countLines() {
  unsigned char *lp; //ポインタ
  uint16_t cnt = 0;  
  
  for (lp = listbuf; *lp; lp += *lp)  cnt++;
  return cnt;   
}

// Insert i-code to the list
// [listbuf]に[ibuf]を挿入
//  [ibuf] : [1:データ長][1:I_NUM][2:行番号][中間コード]
//
void inslist() {
  unsigned char *insp;     // 挿入位置ポインタ
  unsigned char *p1, *p2;  // 移動先と移動元ポインタ
  short len;               // 移動の長さ

  // 空きチェク(これだと、空き不足時に行番号だけ入力時の行削除が出来ないかも.. @たま吉)
  if (getsize() < *ibuf) { // もし空きが不足していたら
    err = ERR_LBUFOF;      // エラー番号をセット
    return;                // 処理を打ち切る
  }

  insp = getlp(getlineno(ibuf)); // 挿入位置ポインタを取得

  // 同じ行番号の行が存在したらとりあえず削除
  if (getlineno(insp) == getlineno(ibuf)) { // もし行番号が一致したら
    p1 = insp;                              // p1を挿入位置に設定
    p2 = p1 + *p1;                          // p2を次の行に設定
    while ((len = *p2) != 0) {              // 次の行の長さが0でなければ繰り返す
      while (len--)                         // 次の行の長さだけ繰り返す
        *p1++ = *p2++;                      // 前へ詰める
    }
    *p1 = 0; // リストの末尾に0を置く
  }

  // 行番号だけが入力された場合はここで終わる
  if (*ibuf == 4) // もし長さが4（[長さ][I_NUM][行番号]のみ）なら
    return;

  // 挿入のためのスペースを空ける

  for (p1 = insp; *p1; p1 += *p1); // p1をリストの末尾へ移動
  len = p1 - insp + 1;             // 移動する幅を計算
  p2 = p1 + *ibuf;                 // p2を末尾より1行の長さだけ後ろに設定
  while (len--)                    // 移動する幅だけ繰り返す
    *p2-- = *p1--;                 // 後ろへズラす

  // 行を転送する
  len = *ibuf;     // 中間コードの長さを設定
  p1 = insp;       // 転送先を設定
  p2 = ibuf;       // 転送元を設定
  while (len--)    // 中間コードの長さだけ繰り返す
    *p1++ = *p2++; // 転送
}

//Listing 1 line of i-code
void putlist(unsigned char* ip) {
  unsigned char i; //ループカウンタ

  while (*ip != I_EOL) { //行末でなければ繰り返す

    //キーワードの処理
    if (*ip < SIZE_KWTBL) { //もしキーワードなら
      c_puts(kwtbl[*ip]); //キーワードテーブルの文字列を表示
      if ( (!nospacea(*ip) || spacef(*(ip+1))) && (*ip != I_COLON) && (*ip != I_SQUOT)) //もし例外にあたらなければ
        c_putch(' '); //空白を表示

      if (*ip == I_REM||*ip == I_SQUOT) { //もし中間コードがI_REMなら
        ip++; //ポインタを文字数へ進める
        i = *ip++; //文字数を取得してポインタをコメントへ進める
        while (i--) //文字数だけ繰り返す
          c_putch(*ip++); //ポインタを進めながら文字を表示
        return; //終了する
      }

      ip++;//ポインタを次の中間コードへ進める
    }
    else

    //定数の処理
    if (*ip == I_NUM) { //もし定数なら
      ip++; //ポインタを値へ進める
      putnum(*ip | *(ip + 1) << 8, 0); //値を取得して表示
      ip += 2; //ポインタを次の中間コードへ進める
      if (!nospaceb(*ip)) //もし例外にあたらなければ
        c_putch(' '); //空白を表示
    }
    else

    //16進定数の処理
    if (*ip == I_HEXNUM) { //もし16進定数なら
      ip++; //ポインタを値へ進める
      c_putch('$'); //空白を表示
      putHexnum(*ip | *(ip + 1) << 8, 2); //値を取得して表示
      ip += 2; //ポインタを次の中間コードへ進める
      if (!nospaceb(*ip)) //もし例外にあたらなければ
        c_putch(' '); //空白を表示
    }
    else
    
    //変数の処理
    if (*ip == I_VAR) { //もし定数なら
      ip++; //ポインタを変数番号へ進める
      c_putch(*ip++ + 'A'); //変数名を取得して表示
      if (!nospaceb(*ip)) //もし例外にあたらなければ
        c_putch(' '); //空白を表示
    }
    else

    //文字列の処理
    if (*ip == I_STR) { //もし文字列なら
      char c; //文字列の括りに使われている文字（「"」または「'」）

      //文字列の括りに使われている文字を調べる
      c = '\"'; //文字列の括りを仮に「"」とする
      ip++; //ポインタを文字数へ進める
      for (i = *ip; i; i--) //文字数だけ繰り返す
        if (*(ip + i) == '\"') { //もし「"」があれば
          c = '\''; //文字列の括りは「'」
          break; //繰り返しを打ち切る
        }

      //文字列を表示する
      c_putch(c); //文字列の括りを表示
      i = *ip++; //文字数を取得してポインタを文字列へ進める
      while (i--) //文字数だけ繰り返す
        c_putch(*ip++); //ポインタを進めながら文字を表示
      c_putch(c); //文字列の括りを表示
      if (*ip == I_VAR || *ip ==I_ELSE) //もし次の中間コードが変数だったら
        c_putch(' '); //空白を表示
    }

    else { //どれにも当てはまらなかった場合
      err = ERR_SYS; //エラー番号をセット
      return; //終了する
    }
  }
}

// Get argument in parenthesis
short getparam() {
  short value; //値

  if (*cip != I_OPEN) { //もし「(」でなければ
    err = ERR_PAREN; //エラー番号をセット
    return 0; //終了
  }
  cip++; //中間コードポインタを次へ進める

  value = iexp(); //式を計算
  if (err) //もしエラーが生じたら
    return 0; //終了
  if (*cip != I_CLOSE) { //もし「)」でなければ
    err = ERR_PAREN; //エラー番号をセット
    return 0; //終了
  }
  cip++; //中間コードポインタを次へ進める

  return value; //値を持ち帰る
}

// Get value
int16_t ivalue() {
  int16_t value; // 値
  uint8_t i;   // 文字数

  switch (*cip) { //中間コードで分岐

  //定数の取得
  case I_NUM:    // 定数の場合
  case I_HEXNUM: // 16進定数
    cip++; //中間コードポインタを次へ進める
    value = *cip | *(cip + 1) << 8; //定数を取得
    cip += 2; //中間コードポインタを定数の次へ進める
    break; //ここで打ち切る

  //+付きの値の取得
  case I_PLUS: //「+」の場合
    cip++; //中間コードポインタを次へ進める
    value = ivalue(); //値を取得
    break; //ここで打ち切る

  //負の値の取得
  case I_MINUS: //「-」の場合
    cip++; //中間コードポインタを次へ進める
    value = 0 - ivalue(); //値を取得して負の値に変換
    break; //ここで打ち切る

  case I_LNOT: //「!」の場合
    cip++; //中間コードポインタを次へ進める
    value = !ivalue(); //値を取得してNOT演算
    break; //ここで打ち切る

  case I_BITREV: // 「~」 ビット反転
    cip++; //中間コードポインタを次へ進める
    value = ~((uint16_t)ivalue()); //値を取得してNOT演算
    break; //ここで打ち切る
  
  //変数の値の取得
  case I_VAR: //変数の場合
    cip++; //中間コードポインタを次へ進める
    value = var[*cip++]; //変数番号から変数の値を取得して次を指し示す
    break; //ここで打ち切る

  //括弧の値の取得
  case I_OPEN: //「(」の場合
    value = getparam(); //括弧の値を取得
    break; //ここで打ち切る

  //配列の値の取得
  case I_ARRAY: //配列の場合
    cip++; //中間コードポインタを次へ進める
    value = getparam(); //括弧の値を取得
    if (err) //もしエラーが生じたら
      break; //ここで打ち切る
    if (value >= SIZE_ARRY) { //もし添え字の上限を超えたら
      err = ERR_SOR; //エラー番号をセット
      break; //ここで打ち切る
    }
    value = arr[value]; //配列の値を取得
    break; //ここで打ち切る

  //関数の値の取得
  case I_RND: //関数RNDの場合
    cip++; //中間コードポインタを次へ進める
    value = getparam(); //括弧の値を取得
    if (err) //もしエラーが生じたら
      break; //ここで打ち切る
    if (value <0) {
      err = ERR_VALUE;
      break;
    }
    value = getrnd(value); //乱数を取得
    break; //ここで打ち切る

  case I_ABS: //関数ABSの場合
    cip++; //中間コードポインタを次へ進める
    value = getparam(); //括弧の値を取得
    if (err) //もしエラーが生じたら
      break; //ここで打ち切る
    if(value < 0) //もし0未満なら
      value *= -1; //正負を反転
    break; //ここで打ち切る

  case I_FREE: //関数FREEの場合
    cip++; //中間コードポインタを次へ進める
    //もし後ろに「()」がなかったら
    if ((*cip != I_OPEN) || (*(cip + 1) != I_CLOSE)) {
      err = ERR_PAREN; //エラー番号をセット
      break; //ここで打ち切る
    }
    cip += 2; //中間コードポインタを「()」の次へ進める
    value = getsize(); //プログラム保存領域の空きを取得
    break; //ここで打ち切る

  case I_INKEY: //関数INKEYの場合
    cip++;  if ((*cip != I_OPEN) || (*(cip + 1) != I_CLOSE)) {      // '()'のチェック
      err = ERR_PAREN; //エラー番号をセット
      break; //ここで打ち切る
    }
    cip += 2; //中間コードポインタを「()」の次へ進める
    value = iinkey(); // キー入力値の取得
    break; //ここで打ち切る

  case I_VPEEK: //関数VPEEKの場合
    cip++; value = ivpeek();
    break; 

  case I_GPEEK: //関数GPEEK(X,Y)
    cip++; value = igpeek();
    break; 

  case I_GINP: //関数GINP(X,Y,W,H)
    cip++; value = iginp();
    break; 

  case I_MAP: //関数MAP(V,L1,H1,L2,H2)
    cip++; value = imap();
    break;
    
  case I_ASC:  // 関数ASC(文字列)の場合
    cip++; if (*cip != I_OPEN) { err = ERR_PAREN; break; }            // '('チェック
    cip++; if ( I_STR != *cip ) { err = ERR_SYNTAX; break; }          // 文字列引数チェック   
    cip++; i = (uint8_t)*cip; if (i != 1) { err = ERR_VALUE; break; } // 文字列長さチェック
    cip++; value = *cip;                                              // 値取得
    cip++; if (*cip != I_CLOSE) { err = ERR_PAREN;break; }            // ')'チェック
    cip++; 
    break;

  case I_TICK: // 関数TICK()の場合
    cip++;              // 中間コードポインタを次へ進める
    if ((*cip == I_OPEN) && (*(cip + 1) == I_CLOSE)) {
      // 引数無し
      value = 0;
      cip+=2;
    } else {
      value = getparam(); // 括弧の値を取得
      if (err)            // もしエラーが生じたら
        break;            // ここで打ち切る
    }

    if(value == 0) {
      value = (millis()/100) & 0x7FFF;        // 下位15ビット(0～32767)
    } else if (value == 1) {
      value = ((millis()/100)>>15) & 0x7FFF;  // 上位15ビット(0～32767)
    } else {
      value = 0;                              // 引数が正しくない
      err = ERR_VALUE;
    }
    break; //ここで打ち切る

  case I_PEEK: // PEEK()関数の場合
    cip++;              // 中間コードポインタを次へ進める
    value = getparam(); // 括弧の値を取得
    if (err)            // もしエラーが生じたら
      break;            // ここで打ち切る
    if (value >= SRAM_SIZE) {
      err = ERR_RANGE;  // SRAMの範囲外を参照した
      break;
    }
    value = *((uint8_t*)((uint32_t)SRAM_TOP +(uint32_t)value));
   
    break; //ここで打ち切る

  case I_ISND: // I2CW()関数の場合
    cip++; value = ii2cw();
    break; 

  case I_IRCV: // I2CW()関数の場合
    cip++; value = ii2cr();
    break; 

  case I_SHIFTIN:
    cip++; value = ishiftIn();
    break; 
  
  // 定数HIGH/LOWの場合
  case I_HIGH: 
   cip++; value = CONST_HIGH;  break;
  case I_LOW:
   cip++;  value = CONST_LOW;  break;

  case I_LSB:
   cip++; value = CONST_LSB;  break;
  case I_MSB:
   cip++; value = CONST_MSB;  break;

  case I_MEM:
   cip++; value = (uint16_t)((uint32_t)mem - (uint32_t)SRAM_TOP);  break;
  
  case I_VRAM:
   cip++; value = (uint16_t)((uint32_t)sc.getScreen() - (uint32_t)SRAM_TOP);  break;

  case I_MVAR:
   cip++; value = (uint16_t)((uint32_t)var - (uint32_t)SRAM_TOP);  break;

  case I_MARRAY:
   cip++; value = (uint16_t)((uint32_t)arr - (uint32_t)SRAM_TOP);  break;
  
  case I_DIN: // DIN(ピン番号)の場合
    cip++; if (*cip != I_OPEN) { err = ERR_PAREN; break; }      // '('チェック
    cip++; value = iexp(); if (err) break;                      // ピン番号取得
    if (*cip != I_CLOSE) { err = ERR_PAREN; break; }            // ')'チェック
    if( value < 0 || value > I_PC15 - I_PA0 )                   // 有効性のチェック
        { err = ERR_VALUE; break; }
    value = digitalRead(value);                                 // 入力値取得
    cip++; 
    break;

  case I_ANA: // ANA(ピン番号)の場合
    cip++; if (*cip != I_OPEN) { err = ERR_PAREN; break; }      // '('チェック
    cip++; value = iexp(); if (err) break;                      // ピン番号取得
    if (*cip != I_CLOSE) { err = ERR_PAREN; break; }            // ')'チェック
    if( value < 0 || value > I_PC15 - I_PA0 )                   // 有効性のチェック
        { err = ERR_VALUE; break; }
    value = analogRead(value);                                  // 入力値取得
    cip++; 
    break;

  case I_EEPREAD: // EEPREAD(アドレス)の場合
    cip++; if (*cip != I_OPEN) { err = ERR_PAREN; break; }      // '('チェック
    cip++; value = iexp(); if (err) break;                      // アドレス取得
    if (*cip != I_CLOSE) { err = ERR_PAREN; break; }            //  ')'チェック
    value = ieepread(value);                                    // 入力値取得
    cip++; 
    break;

  case I_SREAD: // SREAD() シリアルデータ1バイト受信
    cip++;  if ((*cip != I_OPEN) || (*(cip + 1) != I_CLOSE)) {    // '()'のチェック
      err = ERR_PAREN; //エラー番号をセット
      break; //ここで打ち切る
    }
    cip += 2; //中間コードポインタを「()」の次へ進める
    //value = Serial1.read();
    value = sc.Serial_read();
    break; //ここで打ち切る
  
  case I_SREADY:// SREADY() シリアルデータデータチェック
    cip++;  if ((*cip != I_OPEN) || (*(cip + 1) != I_CLOSE)) {    // '()'のチェック
      err = ERR_PAREN; //エラー番号をセット
      break; //ここで打ち切る
    }
    cip += 2; //中間コードポインタを「()」の次へ進める
    //value = Serial1.available();
    value = sc.Serial_available();
    break; //ここで打ち切る

  // 画面サイズ定数の参照
  case I_CW: value = sc.getWidth()   ;cip++; break;
  case I_CH: value = sc.getHeight()  ;cip++; break;
  case I_GW: value = sc.getGWidth()  ;cip++; break;
  case I_GH: value = sc.getGHeight() ;cip++; break;

  // カーソル・スクロール等の方向
  case I_UP:    value = 0   ;cip++; break;
  case I_DOWN:  value = 1   ;cip++; break;
  case I_RIGHT: value = 2   ;cip++; break;
  case I_LEFT:  value = 3   ;cip++; break;

  default: //以上のいずれにも該当しなかった場合
    // 定数ピン番号
    if (*cip >= I_PA0 && *cip <= I_PC15) {
      value = *cip - I_PA0; 
      cip++;
      return value;
    // 定数GPIOモード
    } else  if (*cip >= I_OUTPUT_OPEN_DRAIN && *cip <= I_PWM) {
      value = pinType[*cip - I_OUTPUT_OPEN_DRAIN]; 
      cip++;
      return value;  
    }
    err = ERR_SYNTAX; //エラー番号をセット
    break; //ここで打ち切る
  }
  return value; //取得した値を持ち帰る
}

// multiply or divide calculation
short imul() {
  short value, tmp; //値と演算値

  value = ivalue(); //値を取得
  if (err) //もしエラーが生じたら
    return -1; //終了

  while (1) //無限に繰り返す
  switch(*cip){ //中間コードで分岐

  case I_MUL: //掛け算の場合
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value *= tmp; //掛け算を実行
    break; //ここで打ち切る

  case I_DIV: //割り算の場合
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    if (tmp == 0) { //もし演算値が0なら
      err = ERR_DIVBY0; //エラー番号をセット
      return -1; //終了
    }
    value /= tmp; //割り算を実行
    break; //ここで打ち切る

  case I_DIVR: //剰余の場合
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    if (tmp == 0) { //もし演算値が0なら
      err = ERR_DIVBY0; //エラー番号をセット
      return -1; //終了
    }
    value %= tmp; //割り算を実行
    break; //ここで打ち切る

  case I_LSHIFT: // シフト演算 "<<" の場合
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)<<tmp;
    break; //ここで打ち切る

  case I_RSHIFT: // シフト演算 ">>" の場合
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)>>tmp;
    break; //ここで打ち切る

   case I_AND:  // 算術積(ビット演算)
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)&((uint16_t)tmp);
    break; //ここで打ち切る

   case I_OR:   //算術和(ビット演算)
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)|((uint16_t)tmp);
    break; //ここで打ち切る

   case I_XOR: //非排他OR(ビット演算)
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)^((uint16_t)tmp);
   
  default: //以上のいずれにも該当しなかった場合
    return value; //値を持ち帰る
  } //中間コードで分岐の末尾
}

// add or subtract calculation
short iplus() {
  short value, tmp; //値と演算値

  value = imul(); //値を取得
  if (err) //もしエラーが生じたら
    return -1; //終了

  while (1) //無限に繰り返す
  switch(*cip){ //中間コードで分岐

  case I_PLUS: //足し算の場合
    cip++; //中間コードポインタを次へ進める
    tmp = imul(); //演算値を取得
    value += tmp; //足し算を実行
    break; //ここで打ち切る

  case I_MINUS: //引き算の場合
    cip++; //中間コードポインタを次へ進める
    tmp = imul(); //演算値を取得
    value -= tmp; //引き算を実行
    break; //ここで打ち切る

  default: //以上のいずれにも該当しなかった場合
    return value; //値を持ち帰る
  } //中間コードで分岐の末尾
}

// The parser
short iexp() {
  short value, tmp; //値と演算値

  value = iplus(); //値を取得
  if (err) //もしエラーが生じたら
    return -1; //終了

  // conditional expression 
  while (1) //無限に繰り返す
  switch(*cip){ //中間コードで分岐

  case I_EQ: //「=」の場合
    cip++; //中間コードポインタを次へ進める
    tmp = iplus(); //演算値を取得
    value = (value == tmp); //真偽を判定
    break; //ここで打ち切る

  case I_NEQ: //「<>」の場合
  case I_SHARP: //「#」の場合
    cip++; //中間コードポインタを次へ進める
    tmp = iplus(); //演算値を取得
    value = (value != tmp); //真偽を判定
    break; //ここで打ち切る
  case I_LT: //「<」の場合
    cip++; //中間コードポインタを次へ進める
    tmp = iplus(); //演算値を取得
    value = (value < tmp); //真偽を判定
    break; //ここで打ち切る
  case I_LTE: //「<=」の場合
    cip++; //中間コードポインタを次へ進める
    tmp = iplus(); //演算値を取得
    value = (value <= tmp); //真偽を判定
    break; //ここで打ち切る
  case I_GT: //「>」の場合
    cip++; //中間コードポインタを次へ進める
    tmp = iplus(); //演算値を取得
    value = (value > tmp); //真偽を判定
    break; //ここで打ち切る
  case I_GTE: //「>=」の場合
    cip++; //中間コードポインタを次へ進める
    tmp = iplus(); //演算値を取得
    value = (value >= tmp); //真偽を判定
    break; //ここで打ち切る
 case I_LAND: // AND (論理積)
    cip++; //中間コードポインタを次へ進める
    tmp = iplus(); //演算値を取得
    value = (value && tmp); //真偽を判定
    break; //ここで打ち切る
 case I_LOR: // OR (論理和)
    cip++; //中間コードポインタを次へ進める
    tmp = iplus(); //演算値を取得
    value = (value || tmp); //真偽を判定
    break; //ここで打ち切る
 
  default: //以上のいずれにも該当しなかった場合
    return value; //値を持ち帰る
  } //中間コードで分岐の末尾
}

// PRINT handler
void iprint(uint8_t devno=0) {
  short value; //値
  short len; //桁数
  unsigned char i; //文字数
  
  len = 0; //桁数を初期化
  while (*cip != I_COLON && *cip != I_EOL) { //文末まで繰り返す
    switch (*cip) { //中間コードで分岐

    case I_STR: //文字列の場合
      cip++; //中間コードポインタを次へ進める
      i = *cip++; //文字数を取得
      while (i--) //文字数だけ繰り返す
        c_putch(*cip++, devno); //文字を表示
      break; //打ち切る

    case I_SHARP: //「#」の場合
      cip++; //中間コードポインタを次へ進める
      len = iexp(); //桁数を取得
      if (err) //もしエラーが生じたら
        return; //終了
      break; //打ち切る

    case I_CHR: // CHR$()関数の場合
      cip++;              // 中間コードポインタを次へ進める
      value = getparam(); // 括弧の値を取得
     if (err)             // もしエラーが生じたら
        break;            // ここで打ち切る
     //if(!sc.IS_PRINT(value))
     //   value = 32;
     c_putch(value, devno);
      break; // 打ち切る

    case I_HEX: // HEX$()関数の場合
      cip++;    // 中間コードポインタを次へ進める
      ihex(devno);
      break; //打ち切る

    case I_BIN: // BIN$()関数の場合
      cip++;    // 中間コードポインタを次へ進める
      ibin(devno);
      break; //打ち切る

    case I_DMP: // DMP$()関数
      cip++;    // 中間コードポインタを次へ進める
      idmp(devno);
      break; //打ち切る
    
    case I_ELSE:        // ELSE文がある場合は打ち切る
       newline(devno);  // 改行        
       return;
       break;
       
    default: //以上のいずれにも該当しなかった場合（式とみなす）
      value = iexp();   // 値を取得
      if (err) {        // もしエラーが生じたら
        newline();      // 改行        
        return;         // 終了
      }
      putnum(value, len,devno); // 値を表示
      break;                    // 打ち切る
    } //中間コードで分岐の末尾
    if (err)  {
        newline(devno);   // 改行        
        return;           // 終了      
    }
      
    if (*cip == I_ELSE) {
        newline(devno);   // 改行        
        return;           // 終了       
    } else if (*cip == I_COMMA|*cip == I_SEMI) { // もし',' ';'があったら
      cip++;                              // 中間コードポインタを次へ進める
      if (*cip == I_COLON || *cip == I_EOL || *cip == I_ELSE) //もし文末なら      
        return; //終了
    } else {    //',' ';'がなければ
      if (*cip != I_COLON && *cip != I_EOL) { //もし文末でなければ
        err = ERR_SYNTAX; // エラー番号をセット
        newline(devno);   // 改行        
        return;           // 終了
      }
    }
  } //文末まで繰り返すの末尾
  newline(devno); //改行
}

// INPUT handler
void iinput() {
  short value;          // 値
  short index;          // 配列の添え字or変数番号
  unsigned char i;      // 文字数
  unsigned char prompt; // プロンプト表示フラグ
  short ofvalue;        // オーバーフロー時の設定値
  uint8_t flgofset =0;  // オーバーフロ時の設定値指定あり

  while (1) {           // 無限に繰り返す
    prompt = 1;         // まだプロンプトを表示していない

    // プロンプトが指定された場合の処理
    if(*cip == I_STR){   // もし中間コードが文字列なら
      cip++;             // 中間コードポインタを次へ進める
      i = *cip++;        // 文字数を取得
      while (i--)        // 文字数だけ繰り返す
        c_putch(*cip++); // 文字を表示
      prompt = 0;        // プロンプトを表示した

      if (*cip != I_COMMA) {
        err = ERR_SYNTAX;
        return;
      }
      cip++;
    }

    // 値を入力する処理
    switch (*cip) {         // 中間コードで分岐
    case I_VAR:             // 変数の場合
      cip++;                // 中間コードポインタを次へ進める
      index = *cip;         // 変数番号の取得
      cip++;
     
      // オーバーフロー時の設定値
      if (*cip == I_COMMA) {
        cip++;
        ofvalue = iexp();
        if (err) {
          return;
        }
        flgofset = 1;
      }
      
      if (prompt) {          // もしまだプロンプトを表示していなければ
        c_putch('A'+index);  // 変数名を表示
        c_putch(':');        //「:」を表示
      }
      
      value = getnum();     // 値を入力
      if (err) {            // もしエラーが生じたら
        if (err == ERR_VOF && flgofset) {
          err = ERR_OK;
          value = ofvalue;
        } else {
          return;            // 終了
        }
      }
      var[index] = value;  // 変数へ代入
      break;               // 打ち切る

    case I_ARRAY: // 配列の場合
      cip++;                    // 中間コードポインタを次へ進める
      index = getparam();       // 配列の添え字を取得
      if (err)                  // もしエラーが生じたら
        return;                 // 終了

      if (index >= SIZE_ARRY) { // もし添え字が上限を超えたら
        err = ERR_SOR;          // エラー番号をセット
        return;                 // 終了
      }

      // オーバーフロー時の設定値
      if (*cip == I_COMMA) {
        cip++;
        ofvalue = iexp();
        if (err) {
          return;
        }
        flgofset = 1;
      }

      if (prompt) { // もしまだプロンプトを表示していなければ
        c_puts("@(");     //「@(」を表示
        putnum(index, 0); // 添え字を表示
        c_puts("):");     //「):」を表示
      }
      value = getnum(); // 値を入力
      if (err) {           // もしエラーが生じたら
        if (err == ERR_VOF && flgofset) {
          err = ERR_OK;
          value = ofvalue;
        } else {
          return;            // 終了
        }
      }
      arr[index] = value; //配列へ代入
      break;              // 打ち切る

    default: // 以上のいずれにも該当しなかった場合
      err = ERR_SYNTAX; // エラー番号をセット
      return;            // 終了
    } // 中間コードで分岐の末尾

    //値の入力を連続するかどうか判定する処理
    switch (*cip) { // 中間コードで分岐
    case I_COMMA:    // コンマの場合
      cip++;         // 中間コードポインタを次へ進める
      break;         // 打ち切る
    case I_COLON:    //「:」の場合
    case I_EOL:      // 行末の場合
      return;        // 終了
    default:      // 以上のいずれにも該当しなかった場合
      err = ERR_SYNTAX; // エラー番号をセット
      return;           // 終了
    } // 中間コードで分岐の末尾
  }   // 無限に繰り返すの末尾
}

// Variable assignment handler
void ivar() {
  short value; //値
  short index; //変数番号

  index = *cip++; //変数番号を取得して次へ進む

  if (*cip != I_EQ) { //もし「=」でなければ
    err = ERR_VWOEQ; //エラー番号をセット
    return; //終了
  }
  cip++; //中間コードポインタを次へ進める

  //値の取得と代入
  value = iexp(); //式の値を取得
  if (err) //もしエラーが生じたら
    return; //終了
  var[index] = value; //変数へ代入
}

// Array assignment handler
void iarray() {
  short value; //値
  short index; //配列の添え字

  index = getparam(); //配列の添え字を取得
  if (err) //もしエラーが生じたら
    return; //終了

  if (index >= SIZE_ARRY || index < 0 ) { //もし添え字が上下限を超えたら
    err = ERR_SOR; //エラー番号をセット
    return; //終了
  }

  if (*cip != I_EQ) { //もし「=」でなければ
    err = ERR_VWOEQ; //エラー番号をセット
    return; //終了
  }

  // 例: @(n)=1,2,3,4,5 の連続設定処理
  do {
    cip++;          // 中間コードポインタを次へ進める
    value = iexp(); // 式の値を取得
    if (err)        // もしエラーが生じたら
      return;       // 終了

    if (index >= SIZE_ARRY) { // もし添え字が上限を超えたら
      err = ERR_SOR;          // エラー番号をセット
      return;                 // 終了
    }
    arr[index] = value; //配列へ代入
    index++;
  } while(*cip == I_COMMA);
} 

// LET handler
void ilet() {
  switch (*cip) { //中間コードで分岐
  case I_VAR: // 変数の場合
    cip++;     // 中間コードポインタを次へ進める
    ivar();    // 変数への代入を実行
    break;     // 打ち切る

  case I_ARRAY: // 配列の場合
    cip++;      // 中間コードポインタを次へ進める
    iarray();   // 配列への代入を実行
    break;      // 打ち切る

  default:      // 以上のいずれにも該当しなかった場合
    err = ERR_LETWOV; // エラー番号をセット
    break;            // 打ち切る
  }
}

// 中間コードの実行
// 戻り値      : 次のプログラム実行位置(行の先頭)
unsigned char* iexe() {
  short lineno;            // 行番号
  unsigned char* lp;       // 未確定の（エラーかもしれない）行ポインタ
  uint8_t* newip;          // ELSE文以降の処理対象ポインタ
  short index, vto, vstep; // FOR文の変数番号、終了値、増分
  short condition;         // IF文の条件値
  int16_t prm;             // コマンド引数
  uint8_t c;               // 入力キー
  err = 0;
  while (*cip != I_EOL) { //行末まで繰り返す
  
  //強制的な中断の判定
  c = c_kbhit();
  if (c) { // もし未読文字があったら
      if (c == SC_KEY_CTRL_C || c==27 ) { // 読み込んでもし[ESC],［CTRL_C］キーだったら
        err = ERR_CTR_C;                  // エラー番号をセット
        prevPressKey = 0;
        break;
      } else {
        prevPressKey = c;
      }
    }

    //中間コードを実行
    switch (*cip) { //中間コードで分岐
    
    case I_STR:  // 文字列の場合(ラベル)
     cip++;
     cip+= *cip+1;
     break;
    
    case I_GOTO: // GOTOの場合
      cip++;
      if (*cip == I_STR) {                         
        lp = getlpByLabel(cip);                   // ラベル参照による分岐先取得
        if (lp == NULL) {
          err = ERR_ULN;                          // エラー番号をセット
          break; 
        }  
      } else {
        lineno = iexp(); if (err)  break;         // 引数の行番号取得
        lp = getlp(lineno);                       // 分岐先のポインタを取得
        if (lineno != getlineno(lp)) {            // もし分岐先が存在しなければ
          err = ERR_ULN;                          // エラー番号をセット
          break; 
        }
      }
      clp = lp;                                 // 行ポインタを分岐先へ更新
      cip = clp + 3;                            // 中間コードポインタを先頭の中間コードに更新
      break; //打ち切る

    case I_GOSUB: // GOSUBの場合
      cip++;
      if (*cip == I_STR) {                         
        lp = getlpByLabel(cip);                   // ラベル参照による分岐先取得
        if (lp == NULL) {
          err = ERR_ULN;                          // エラー番号をセット
          break; 
        }  
      } else {
        lineno = iexp(); if (err)  break;  // 引数の行番号取得
        lp = getlp(lineno);                       // 分岐先のポインタを取得
        if (lineno != getlineno(lp)) {            // もし分岐先が存在しなければ
          err = ERR_ULN;                          // エラー番号をセット
          break; 
        }
      }
      
      //ポインタを退避
      if (gstki > SIZE_GSTK - 2) {              // もしGOSUBスタックがいっぱいなら
        err = ERR_GSTKOF;                       // エラー番号をセット
        break;
      }
      gstk[gstki++] = clp;                      // 行ポインタを退避
      gstk[gstki++] = cip;                      // 中間コードポインタを退避

      clp = lp;                                 // 行ポインタを分岐先へ更新
      cip = clp + 3;                            // 中間コードポインタを先頭の中間コードに更新
      break; 

    case I_RETURN: // RETURNの場合
      if (gstki < 2) {    // もしGOSUBスタックが空なら
        err = ERR_GSTKUF; // エラー番号をセット
        break; 
      }

      cip = gstk[--gstki]; //行ポインタを復帰
      clp = gstk[--gstki]; //中間コードポインタを復帰
      break;

    case I_FOR: // FORの場合
      cip++; // 中間コードポインタを次へ進める

      // 変数名を取得して開始値を代入（例I=1）
      if (*cip++ != I_VAR) { // もし変数がなかったら
        err = ERR_FORWOV;    // エラー番号をセット
        break;
      }
      index = *cip; // 変数名を取得
      ivar();       // 代入文を実行
      if (err)      // もしエラーが生じたら
        break; 

      // 終了値を取得（例TO 5）
      if (*cip == I_TO) { // もしTOだったら
        cip++;             // 中間コードポインタを次へ進める
        vto = iexp();      // 終了値を取得
      } else {             // TOではなかったら
        err = ERR_FORWOTO; //エラー番号をセット
        break; 
      }

      // 増分を取得（例STEP 1）
      if (*cip == I_STEP) { // もしSTEPだったら
        cip++;              // 中間コードポインタを次へ進める
        vstep = iexp();     // 増分を取得
      } else                // STEPではなかったら
        vstep = 1;          // 増分を1に設定

      // もし変数がオーバーフローする見込みなら
      if (((vstep < 0) && (-32767 - vstep > vto)) ||
        ((vstep > 0) && (32767 - vstep < vto))){
        err = ERR_VOF; //エラー番号をセット
        break;
      }

      // 繰り返し条件を退避
      if (lstki > SIZE_LSTK - 5) { // もしFORスタックがいっぱいなら
        err = ERR_LSTKOF;          // エラー番号をセット
        break; 
      }
      lstk[lstki++] = clp; // 行ポインタを退避
      lstk[lstki++] = cip; // 中間コードポインタを退避

      // FORスタックに終了値、増分、変数名を退避
      // Special thanks hardyboy
      lstk[lstki++] = (unsigned char*)(uintptr_t)vto;
      lstk[lstki++] = (unsigned char*)(uintptr_t)vstep;
      lstk[lstki++] = (unsigned char*)(uintptr_t)index;
      break; // 打ち切る

    case I_NEXT: // NEXTの場合
      cip++; // 中間コードポインタを次へ進める
      if (lstki < 5) {    // もしFORスタックが空なら
        err = ERR_LSTKUF; // エラー番号をセット
        break;
      }

      // 変数名を復帰
      index = (short)(uintptr_t)lstk[lstki - 1]; // 変数名を復帰
      if (*cip++ != I_VAR) {                     // もしNEXTの後ろに変数がなかったら
        err = ERR_NEXTWOV;                       // エラー番号をセット
        break;
      }
      if (*cip++ != index) { // もし復帰した変数名と一致しなかったら
        err = ERR_NEXTUM;    // エラー番号をセット
        break;
      }

      vstep = (short)(uintptr_t)lstk[lstki - 2]; // 増分を復帰
      var[index] += vstep;                       // 変数の値を最新の開始値に更新
      vto = (short)(uintptr_t)lstk[lstki - 3];   // 終了値を復帰

      // もし変数の値が終了値を超えていたら
      if (((vstep < 0) && (var[index] < vto)) ||
        ((vstep > 0) && (var[index] > vto))) {
        lstki -= 5;  // FORスタックを1ネスト分戻す
        break; 
      }

      // 開始値が終了値を超えていなかった場合
      cip = lstk[lstki - 4]; //行ポインタを復帰
      clp = lstk[lstki - 5]; //中間コードポインタを復帰
      break; //打ち切る

    case I_IF: // IFの場合
      cip++;   // 中間コードポインタを次へ進める
      condition = iexp(); // 真偽を取得
      if (err) {          // もしエラーが生じたら
        err = ERR_IFWOC;  // エラー番号をセット
        break; 
      }
      if (condition) {    // もし真なら
        break;            // 打ち切る（次の文を実行する）
      } else { 
        // 偽の場合の処理
        // ELSEがあるかチェックする
        // もしELSEより先にIFが見つかったらELSE無しとする        
        // ELSE文が無い場合の処理はREMと同じ
        newip = getELSEptr(cip);
        if (newip != NULL) {
          cip = newip;
          break;
        }
        while (*cip != I_EOL) // I_EOLに達するまで繰り返す
        cip++;                // 中間コードポインタを次へ進める        
      }
      break;
    case I_ELSE:  // 単独のELSEの場合     
    case I_SQUOT: // 'の場合
    case I_REM:   // REMの場合
      while (*cip != I_EOL) // I_EOLに達するまで繰り返す
        cip++;              // 中間コードポインタを次へ進める
      break;

    case I_END: // ENDの場合
      while (*clp)    // 行の終端まで繰り返す
        clp += *clp;  // 行ポインタを次へ進める
      return clp;     // 行ポインタを持ち帰る

    case I_CLS: // CLSの場合
        cip++;
        icls();
        break;

    case I_WAIT: // WAITの場合
      cip++;
      prm = iexp();
      if(err) return NULL;
      if(prm < 0) {
        err = ERR_VALUE;
        break;
      }
      iwait(prm);
      break;    

    case I_LOCATE: // LOCATEの場合
      cip++;
      ilocate();
      break;    
     case I_COLOR: // COLORの場合
      cip++;
      icolor();
      break;    
     case I_ATTR: // ATTRの場合
      cip++;
      iattr();
      break;    

    //一般の文に相当する中間コードの照合と処理
    case I_VAR: // 変数の場合（LETを省略した代入文）
      cip++;    // 中間コードポインタを次へ進める
      ivar();   // 代入文を実行
      break; 
    case I_ARRAY: // 配列の場合（LETを省略した代入文）
      cip++;      // 中間コードポインタを次へ進める
      iarray();   // 代入文を実行
      break; 
    case I_LET: // LETの場合
      cip++;    // 中間コードポインタを次へ進める
      ilet();   // LET文を実行
      break;
    case I_QUEST:
    case I_PRINT: // PRINTの場合
      cip++;      // 中間コードポインタを次へ進める
      iprint();   // PRINT文を実行
      break; 
    case I_INPUT: // INPUTの場合
      cip++;      // 中間コードポインタを次へ進める
      sc.show_curs(1);
      iinput();   // INPUT文を実行
      sc.show_curs(0);
      break;

    case I_GPIO:  // GPIO
      cip++;
      igpio();
      break;    
      
    case I_DOUT: // OUT
      cip++;
      idwrite();
      break;    

    case I_POUT: // PWM
      cip++;
      ipwm();
      break;        
    
    case I_SHIFTOUT: // ShiftOut
      cip++;
      ishiftOut();
      break;

    case I_POKE: // POKEコマンド
      cip++;
      ipoke();
      break;

    case I_SETDATE: // SETDATEコマンド
      cip++;
      isetDate();
      break;
    
    case I_GETDATE: // GETDATEコマンド
      cip++;
      igetDate();
      break;

    case I_GETTIME: // GETDATEコマンド
      cip++;
      igetTime();
      break;

    case I_DATE:   // DATEコマンド
      cip++;
      idate();
      break;

    case I_RESETTICK: // RESETTICKコマンド
      cip++;
      resetTick();
      break;

    case I_REFLESH:  // REFLESHコマンド 画面再表示
      cip++;
      sc.refresh();
      break;

    case I_EEPFORMAT: // EPPFORMAT EEPROM(エミュレーション)の初期化
      cip++;
      ieepformat();
      break;

    case I_EEPWRITE:  // EEPWRITE コマンド
      cip++;
      ieepwrite();
      break;

   case I_PSET:       // PSETコマンド ドットの描画
      cip++;
      ipset();
      break;

   case I_LINE:       // LINEコマンド 直線の描画
      cip++;
      iline();
      break;

   case I_CIRCLE:     // CIRCLEコマンド 円の描画
      cip++;
      icircle();
      break;

   case I_RECT:       // 四角の表示
      cip++;
      irect();
      break;

   case I_BITMAP:     // ビットマップの描画
      cip++;
      ibitmap();
      break;

   case I_CSCROLL:    // キャラクタスクロール
      cip++;
      icscroll();
      break;

   case I_GSCROLL:    // グラフィックスクロール
      cip++;
      igscroll();
      break;
      
   case I_SWRITE:     // シリアル1バイト出力
      cip++;
      iswrite();
      break;

   case I_SPRINT:
      cip++;
      iprint(1);
      break;

   case I_GPRINT:
      cip++;
      igprint();
      break;

   case I_SOPEN:
      cip++;
      isopen();
      break;

   case I_SCLOSE:
      cip++;
      isclose();
      break;

   case I_SMODE:
      cip++;
      ismode();
      break;
   
   case I_TONE:
      cip++;
      itone();
      break;
   
   case I_NOTONE:
      cip++;
      inotone();
      break;

   case I_LRUN:
      cip++;
      if(ilrun()) {
        return cip;
      }
      break;   

    case I_NEW:   // 中間コードがNEWの場合
    case I_LIST:  // 中間コードがLISTの場合
    case I_RUN:   // 中間コードがRUNの場合
    case I_RENUM: // 中間コードがRENUMの場合
    case I_LOAD:
    case I_SAVE:
    case I_FILES:
    case I_EXPORT:
    case I_CONFIG:
    case I_SAVECONFIG:
    case I_DELETE:
        
      err = ERR_COM; //エラー番号をセット
      return NULL; //終了
    case I_COLON: // 中間コードが「:」の場合
      cip++;      // 中間コードポインタを次へ進める
      break; 
    default: // 以上のいずれにも該当しない場合
      err = ERR_SYNTAX; //エラー番号をセット
      break;
    } //中間コードで分岐の末尾

    if (err) //もしエラーが生じたら
      return NULL; //終了
  } //行末まで繰り返すの末尾
  return clp + *clp;
}

// RUN command handler
void irun() {
  unsigned char* lp; // 行ポインタの一時的な記憶場所
  gstki = 0;         // GOSUBスタックインデクスを0に初期化
  lstki = 0;         // FORスタックインデクスを0に初期化
  clp = listbuf;     // 行ポインタをプログラム保存領域の先頭に設定

  while (*clp) {     // 行ポインタが末尾を指すまで繰り返す
    cip = clp + 3;   // 中間コードポインタを行番号の後ろに設定
    lp = iexe();     // 中間コードを実行して次の行の位置を得る
    if (err)         // もしエラーを生じたら
      return;    
    clp = lp;         // 行ポインタを次の行の位置へ移動
  }
}

// LISTコマンド
void ilist() {
  short lineno = 0;          // 表示開始行番号
  short endlineno = 32767;   // 表示終了行番号
  short prnlineno;           // 出力対象行番号

  //表示開始行番号の設定
  if (*cip == I_NUM) {         // もしLIST命令に引数があったら
    lineno = getlineno(cip);   // 引数を読み取って表示開始行番号とする
    cip+=3;
    if (*cip == I_COMMA) {
        cip++;                         // カンマをスキップ
        if (*cip == I_NUM) {           // 表示終了行番号の指定がある
           endlineno = getlineno(cip); // 引数を読み取って表示終了行番号とする
           cip+=3;
        } else {
           err = ERR_SYNTAX;           // カンマありで引数なしの場合はエラーとする
           return;
       }
    }
  } else if (*cip != I_EOL) {
     err = ERR_SYNTAX;           // 不当な引数を指定している
     return;      
  }

  if ( endlineno < lineno ) {
     err = ERR_VALUE;           // 表示終了行番号が表示開始行番号より小さい場合はエラーとする
     return;     
  }
  
  //行ポインタを表示開始行番号へ進める
  for ( clp = listbuf; *clp && (getlineno(clp) < lineno); clp += *clp); 
  
  //リストを表示する
  while (*clp) {               // 行ポインタが末尾を指すまで繰り返す
    prnlineno = getlineno(clp);// 行番号取得
    if (prnlineno > endlineno) // 表示終了行番号に達したら抜ける
       break; 
    putnum(prnlineno, 0);      // 行番号を表示
    c_putch(' ');              // 空白を入れる
    putlist(clp + 3);          // 行番号より後ろを文字列に変換して表示
    if (err)                   // もしエラーが生じたら
      break;                   // 繰り返しを打ち切る
    newline();                 // 改行
    clp += *clp;               // 行ポインタを次の行へ進める
  }
}



// フラッシュメモリ内保存プログラムのエクスポート
void iexport() {
  uint8_t* exclp;
  int16_t endlineno = 32767;   // 表示終了行番号
  int16_t prnlineno;           // 出力対象行番号
  int16_t s_pno = 0;
  int16_t e_pro = FLASH_SAVE_NUM-1;

  //表示開始番号の設定
  if (*cip == I_NUM) {         // もしLIST命令に引数があったら
    s_pno = getlineno(cip);   // 引数を読み取って表示開始行番号とする
    e_pro = s_pno;
    cip+=3;
    if (*cip == I_COMMA) {
        cip++;                         // カンマをスキップ
        if (*cip == I_NUM) {           // 表示終了番号の指定がある
           e_pro = getlineno(cip); // 引数を読み取って表示終了行番号とする
           cip+=3;
        } else {
           err = ERR_SYNTAX;           // カンマありで引数なしの場合はエラーとする
           return;
       }
    }
  } else if (*cip != I_EOL) {
     err = ERR_SYNTAX;           // 不当な引数を指定している
     return;      
  }

  if ( e_pro < s_pno ) {
     err = ERR_VALUE;           // 表示終了番号が表示開番号より小さい場合はエラーとする
     return;     
  }
  
  for (uint16_t i =s_pno; i <= e_pro;i++) {
    exclp = (uint8_t*)(FLASH_START_ADDRESS + FLASH_PAGE_SIZE*(FLASH_PRG_START_PAGE+ i*FLASH_PAGE_PAR_PRG));
    if (*exclp == 0 || *exclp == 0xff) {
       continue;
    }
    // リストの表示
    c_puts("NEW"); newline();                 // "NEWの表示"

    //リストを表示する
    while (*exclp) {                // 行ポインタが末尾を指すまで繰り返す
      prnlineno = getlineno(exclp); // 行番号取得
      if (prnlineno > endlineno)    // 表示終了行番号に達したら抜ける
         break; 
      putnum(prnlineno, 0);         // 行番号を表示
      c_putch(' ');                 // 空白を入れる
      putlist(exclp + 3);           // 行番号より後ろを文字列に変換して表示
      if (err)                      // もしエラーが生じたら
        break;                      // 繰り返しを打ち切る
      newline();                    // 改行
      exclp += *exclp;              // 行ポインタを次の行へ進める
    }
    err = 0;
    c_puts("SAVE "); putnum(i, 0); newline(); // "SAVE XX"の表示
    newline();
  }
}


//NEW command handler
void inew(uint8_t mode = 0) {
  unsigned char i; //ループカウンタ

  //変数と配列の初期化
  if (mode == 0) {
    for (i = 0; i < 26; i++) //変数の数だけ繰り返す
      var[i] = 0; //変数を0に初期化
    for (i = 0; i < SIZE_ARRY; i++) //配列の数だけ繰り返す
      arr[i] = 0; //配列を0に初期化
  }
  //実行制御用の初期化
  gstki = 0; //GOSUBスタックインデクスを0に初期化
  lstki = 0; //FORスタックインデクスを0に初期化
  *listbuf = 0; //プログラム保存領域の先頭に末尾の印を置く
  clp = listbuf; //行ポインタをプログラム保存領域の先頭に設定
}
	
// RENUME command handler
void irenum() {
  uint16_t startLineNo = 10;  // 開始行番号
  uint16_t increase = 10;     // 増分
  uint8_t* ptr;               // プログラム領域参照ポインタ
  uint16_t len;               // 行長さ
  uint16_t i;                 // 中間コード参照位置
  uint16_t newnum;            // 新しい行番号
  uint16_t num;               // 現在の行番号
  uint16_t index;             // 行インデックス
  uint16_t cnt;               // プログラム行数
  
  // 開始行番号、増分引数チェック
  if (*cip == I_NUM) {               // もしRENUMT命令に引数があったら
    startLineNo = getlineno(cip);    // 引数を読み取って開始行番号とする
    cip+=3;
	  if (*cip == I_COMMA) {
		    cip++;                        // カンマをスキップ
		    if (*cip == I_NUM) {          // 増分指定があったら
	         increase = getlineno(cip); // 引数を読み取って増分とする
		    } else {
		       err = ERR_SYNTAX;          // カンマありで引数なしの場合はエラーとする
		       return;
		   }
 	  }
  }

  // 引数の有効性チェック
  cnt = countLines()-1;
  if (startLineNo <= 0 || increase <= 0) {
	  err = ERR_VALUE;
	  return;		
  }
  if (startLineNo + increase * cnt > 32767) {
    err = ERR_VALUE;
    return;       
  }

  // ブログラム中のGOTOの飛び先行番号を付け直す
  for (  clp = listbuf; *clp ; clp += *clp) {
     ptr = clp;
     len = *ptr;
     ptr++;
     i=0;
     // 行内検索
  	 while( i < len-1 ) {
  	    switch(ptr[i]) {
  	    case I_GOTO:  // GOTO命令
  	    case I_GOSUB: // GOSUB命令
  	      i++;
  	      if (ptr[i] == I_NUM) {
  	    		num = getlineno(&ptr[i]);    // 現在の行番号を取得する
  	    		index = getlineIndex(num);   // 行番号の行インデックスを取得する
  	    		if (index == 32767) {
               // 該当する行が見つからないため、変更は行わない
  	    		   i+=3;
  	    		   continue;
  	    		} else {
  	    		   // とび先行番号を付け替える
  	    		   newnum = startLineNo + increase*index;
  	    		   ptr[i+2] = newnum>>8;
  	    		   ptr[i+1] = newnum&0xff;
  	    		   i+=3;
  	    		   continue;
  	    		}
  	      }	
  	      break;
  		case I_STR:  // 文字列
  		  i++;
  		  i+=ptr[i]; // 文字列長分移動
  		  break;
  		case I_NUM:  // 定数
      case I_HEXNUM: 
  		  i+=3;      // 整数2バイト+中間コード1バイト分移動
  		  break;
  		case I_VAR:  // 変数
  		  i+=2;      // 変数名
  		  break;
  		default:     // その他
  		  i++;
  		  break;
  	  }
  	}
  }
  
  // 各行の行番号の付け替え
  index = 0;
  for (  clp = listbuf; *clp ; clp += *clp ) {
     newnum = startLineNo + increase * index;
  	 *(clp+1)  = newnum&0xff;
   	 *(clp+2)  = newnum>>8;
     index++;
  }
}

// CONFIGコマンド
// CONFIG 項目番号,設定値
void iconfig() {
  int16_t itemNo;
  int16_t value;
  
  if (*cip != I_NUM) {
    err = ERR_SYNTAX;
    return;
  }

  itemNo = getlineno(cip);
  cip+=3;
  if (*cip != I_COMMA) {
    err = ERR_SYNTAX;
    return;
  }
  cip++;
  value = iexp();
  if (err) 
    return;
  switch(itemNo) {
  case 0: // NTSC補正
    if (value <0 || value >2)  {
      err = ERR_VALUE;
    } else {
      sc.adjustNTSC(value);
      CONFIG.NTSC = value;
    }
    break;
  case 1: // キーボード補正
    if (value <0 || value >1)  {
      err = ERR_VALUE;
    } else {
      sc.reset_kbd(value);
      CONFIG.KEYBOARD = value;
    }
    break;
  
  case 2: // プログラム自動起動番号設定
    if (value < -1 || value >9)  {
      err = ERR_VALUE;
    } else {
      CONFIG.STARTPRG = value;
    }
    break;
  default:
    err = ERR_VALUE;
    break;     
 }
}

void isaveconfig() {
  saveConfig();
}

void iloadconfig() {
  loadConfig();
}

// プログラム保存 SAVE 保存領域番号
void isave() {
  int16_t prgno = 0;
  uint32_t flash_adr[FLASH_PAGE_PAR_PRG];
  uint8_t* sram_adr;
  
  // 引数の取得
  if (*cip == I_EOL) {
    prgno = 0;    
  } else if (*cip == I_NUM) {
    prgno = getlineno(cip);
  } else {
    err = ERR_SYNTAX;
    return;
  }

  // 引数の範囲チェック
  if (prgno < 0 || prgno >= FLASH_SAVE_NUM) {
    err = ERR_VALUE;
    return;
  }

  for (uint8_t i=0;i < FLASH_PAGE_PAR_PRG; i++) {
    flash_adr[i] = FLASH_START_ADDRESS + FLASH_PAGE_SIZE*(FLASH_PRG_START_PAGE+ prgno*FLASH_PAGE_PAR_PRG+i);
  }

  // 4ページ分(4096)の保存
  TFlash.unlock();
  for (uint8_t i=0;i < FLASH_PAGE_PAR_PRG; i++) {
    TFlash.eracePage(flash_adr[i]);
    TFlash.write((uint16_t*)flash_adr[i], listbuf+FLASH_PAGE_SIZE*i, FLASH_PAGE_SIZE);
  }
  TFlash.lock();
}

uint8_t loadPrg(uint16_t prgno) {
  uint32_t flash_adr;
  flash_adr = FLASH_START_ADDRESS + FLASH_PAGE_SIZE*(FLASH_PRG_START_PAGE+ prgno*FLASH_PAGE_PAR_PRG);

  // 指定領域に保存されているかチェックする
  if ( *((uint8_t*)flash_adr) == 0xff && *((uint8_t*)flash_adr+1) == 0xff) {
    err = ERR_NOPRG;
    return 1;
  }
  // 現在のプログラムの削除とロード
  inew();
  memcpy(listbuf , (uint8_t*)flash_adr, FLASH_PAGE_SIZE*FLASH_PAGE_PAR_PRG);
  return 0;
}


// プログラム保存 LOAD 保存領域番号
void iload() {
  int16_t prgno = 0;
  uint32_t flash_adr;

  // 引数の取得
  if (*cip == I_EOL) {
    prgno = 0;    
  } else if (*cip == I_NUM) {
    prgno = getlineno(cip);
  } else {
    err = ERR_SYNTAX;
    return;
  }
  // 引数の範囲チェック
  if (prgno < 0 || prgno >= FLASH_SAVE_NUM) {
    err = ERR_VALUE;
    return;
  }
  loadPrg(prgno);
/*
  flash_adr = FLASH_START_ADDRESS + FLASH_PAGE_SIZE*(FLASH_PRG_START_PAGE+ prgno*FLASH_PAGE_PAR_PRG);

  // 指定領域に保存されているかチェックする
  if ( *((uint8_t*)flash_adr) == 0xff && *((uint8_t*)flash_adr+1) == 0xff) {
    err = ERR_NOPRG;
    return;
  }

  // 現在のプログラムの削除
  inew();
  memcpy(listbuf , (uint8_t*)flash_adr, FLASH_PAGE_SIZE*FLASH_PAGE_PAR_PRG);
*/
}

// 指定行の削除
// DELETE 行番号
// DELETE 開始行番号,終了行番号
void idelete() {
 int16_t sNo;
 int16_t eNo;
 uint8_t  *lp;      // 削除位置ポインタ 
 uint8_t *p1, *p2;  // 移動先と移動元ポインタ
 int16_t len;       // 移動の長さ
   
 // 引数の取得
 if (*cip == I_NUM) {
    sNo = getlineno(cip);
    cip+=3;
  } else {
    err = ERR_SYNTAX;
    return;
  }
  if (*cip == I_EOL) {
    eNo = sNo;
  } else if (*cip == I_COMMA) {
    cip++;
   if (*cip == I_NUM) {
      eNo = getlineno(cip);
      cip+=3;
    } else {
      err = ERR_SYNTAX;
      return;
    }    
  }

  if (eNo < sNo) {
    err = ERR_VALUE;
    return;
  }

  if (eNo == sNo) {
      lp = getlp(eNo); // 削除位置ポインタを取得    
      if (getlineno(lp) == sNo) {
        // 削除
        p1 = lp;                              // p1を挿入位置に設定
        p2 = p1 + *p1;                        // p2を次の行に設定
        while ((len = *p2) != 0) {            // 次の行の長さが0でなければ繰り返す
          while (len--)                       // 次の行の長さだけ繰り返す
            *p1++ = *p2++;                    // 前へ詰める
        }
        *p1 = 0; // リストの末尾に0を置く
      }
  } else {
    for (uint16_t i = sNo; i <= eNo;i++) {
      lp = getlp(i); // 削除位置ポインタを取得
      if (getlineno(lp) == i) {               // もし行番号が一致したら
        p1 = lp;                              // p1を挿入位置に設定
        p2 = p1 + *p1;                        // p2を次の行に設定
        while ((len = *p2) != 0) {            // 次の行の長さが0でなければ繰り返す
          while (len--)                       // 次の行の長さだけ繰り返す
            *p1++ = *p2++;                    // 前へ詰める
        }
        *p1 = 0; // リストの末尾に0を置く
      }
    }
  }
}

// プログラム保存 LOAD 保存領域番号
void ifiles() {
  uint32_t flash_adr;
  uint8_t* save_clp;

  save_clp = clp;
  for (uint8_t i=0 ; i < FLASH_SAVE_NUM; i++) {    
    flash_adr = FLASH_START_ADDRESS + FLASH_PAGE_SIZE*(FLASH_PRG_START_PAGE+ i*FLASH_PAGE_PAR_PRG);
    putnum(i,1);
    c_puts(":");
    if ( *((uint8_t*)flash_adr) == 0xff && *((uint8_t*)flash_adr+1) == 0xff) {
      c_puts("(none)");        
    } else {
      clp = (uint8_t*)flash_adr;
      if (*clp) {
        putnum(getlineno(clp), 0); // 行番号を表示
        c_puts(" ");
        putlist(clp + 3);          // 行番号より後ろを文字列に変換して表示
      } else {
        c_puts("(none)");        
      }
    } 
    newline();
  }
  clp = save_clp;
}

//Command precessor
uint8_t icom() {
  uint8_t rc = 1;
  uint8_t v;
  cip = ibuf;          // 中間コードポインタを中間コードバッファの先頭に設定

  switch (*cip) {      // 中間コードポインタが指し示す中間コードによって分岐
  case I_NEW:          // I_NEWの場合（NEW命令）
    cip++;             // 中間コードポインタを次へ進める
    if (*cip == I_EOL) // もし行末だったら
      inew();          // NEW命令を実行
    else               // 行末でなければ
      err = ERR_SYNTAX;// エラー番号をセット
    break;
  case I_LIST:         // I_LISTの場合（LIST命令）
    cip++;             // 中間コードポインタを次へ進める
      ilist();         // LIST命令を実行
    break;

  case I_EXPORT:       // EXPORTコマンド
    cip++;             // 中間コードポインタを次へ進める
    iexport();         // EXPORT命令を実行
    break;

  case I_LRUN:
    cip++;             // 中間コードポインタを次へ進める
    v = ilrun();
    if (v) {
      sc.show_curs(0);
      irun();           // RUN命令を実行
      sc.show_curs(1);
    } else {
      err = ERR_VALUE;
    }
    break;
   
  case I_RUN: //I_RUNの場合（RUN命令）
    cip++;         // 中間コードポインタを次へ進める
    sc.show_curs(0);
    irun();        // RUN命令を実行
    sc.show_curs(1);
    break;
  case I_RENUM: // I_RENUMの場合
    cip++;
    if (*cip == I_EOL || *(cip + 3) == I_EOL || *(cip + 7) == I_EOL)
      irenum(); 
    else
      err = ERR_SYNTAX;
  	break;
  case I_DELETE:
    cip++;
    idelete();
    break;    
  case I_SAVE:
    cip++;
    isave();
    break;
  case I_LOAD:
    cip++;
    iload();
    break;
  case I_FILES:
    cip++;
    ifiles();
    break;
  case I_CONFIG:
    cip++;
    iconfig();
    break;
  case I_SAVECONFIG:
    cip++;
    isaveconfig();
    break;

  case I_OK: // I_OKの場合
    cip++; rc = 0;
    break;
  default:            // どれにも該当しない場合
    sc.show_curs(0);
    iexe();           // 中間コードを実行
    sc.show_curs(1);
    break;
  }
  return rc;
}

// 画面クリア
void icls() {
  sc.cls();
  sc.locate(0,0);
}

// 時間待ち
void iwait(uint16_t tm) {
  delay(tm);
}

// カーソル移動 LOCATE x,y
void ilocate() {
  int16_t x,  y;

  x = iexp(); if(err) return ;                      // xの取得
  if(*cip != I_COMMA){ err = ERR_SYNTAX; return; }  // ','のチェック
  
  cip++; y = iexp();  if(err) return ;              // yの取得

  if ( x >= sc.getWidth() )   // xの有効範囲チェック
     x = sc.getWidth() - 1;
  else if (x < 0)  x = 0;
  
  if( y >= sc.getHeight() )   // yの有効範囲チェック
     y = sc.getHeight() - 1;
  else if(y < 0)   y = 0;

  // カーソル移動
  sc.locate((uint16_t)x, (uint16_t)y);
}

// 文字色の指定 COLOLR fc,bc
void icolor() {
int16_t fc,  bc;

  fc = iexp();  if(err) return ; // 文字色の取得
  if(*cip != I_COMMA) {          // ','チェック
    bc = 0;                      // 省略時の値セット
  } else {                       // 背景色の取得
    cip++; bc = iexp(); if(err) return ;
  }

  // 文字色範囲チェック
  if (fc < 0 || fc > 8 || bc < 0 || bc > 8) {
    err = ERR_VALUE;
    return;
  }
  
  // 文字色の設定
  sc.setColor((uint16_t)fc, (uint16_t)bc);  
}

// 文字属性の指定 ATTRコマンド
void iattr() {
 int16_t attr;

  // 文字属性の取得
  attr = iexp(); if(err) return ; 

  // 文字属性の有効性チェック
  if (attr < 0 || attr > 4) {
    err = ERR_VALUE;
    return;
  }
  sc.setAttr(attr);               // 文字属性のセット
}

// キー入力文字コードの取得 INKEY()関数
int16_t iinkey() {
  int16_t rc = 0;
  
  if (prevPressKey) {
    // 一時バッファに入力済キーがあればそれを使う
    rc = prevPressKey;
    prevPressKey = 0;
  } else if (c_kbhit( )) {
    // キー入力
    rc = c_getch();
  }
  return rc;
}

// スクリーン座標の文字コードの取得 'VPEEK(X,Y)'
int16_t ivpeek() {
  short value; // 値
  short x, y;  // 座標
  
  if (*cip != I_OPEN)  { err = ERR_PAREN; return 0; }  // '('のチェック
  cip++; x = iexp(); if (err) return 0;                // x座標取得
  if (*cip != I_COMMA) { err = ERR_SYNTAX; return 0; } // ','のチェック
  cip++; y = iexp();if (err) return 0;                 // y座標取得
  if (*cip != I_CLOSE) { err = ERR_PAREN;  return 0; } // ')'のチェック
  cip++; 
  value = (x < 0 || y < 0) ? 0: sc.vpeek(x, y);        // 座標位置文字コードセット
  return value;
}

// GPIO ピン機能設定
void igpio() {
  int16_t pinno;       // ピン番号
  WiringPinMode pmode; // 入出力モード
  uint8_t flgok = false;

  // 入出力ピンの指定
  if ( getParam(pinno, 0, I_PC15-I_PA0, true) ) return; // ピン番号取得
  pmode = (WiringPinMode)iexp();  if(err) return ;      // 入出力モードの取得

  // ピンモードの設定
  if (pmode == PWM) {
    for (uint8_t i=0; i < sizeof(pwmpins); i++) {
      if (pinno == pwmpins[i]) {
        flgok = true;
        break;
      }
    }  
    if (flgok == false) {
      err = ERR_VALUE;
      return;
    } 
    pinMode(pinno, pmode);
    pwmWrite(pinno,0);
  } else {
    pinMode(pinno, pmode);    
  }
}

// GPIO ピンデジタル出力
void idwrite() {
  int16_t pinno,  data;

  if ( getParam(pinno, 0, I_PC15-I_PA0, true) ) return; // ピン番号取得
  if ( getParam(data, false) ) return;                  // データ指定取得
  data = data ? HIGH: LOW;
  
  // ピンモードの設定
  digitalWrite(pinno, data);
}

//
// PWM出力
// 引数
//   pin     PWM出力ピン
//   freq    出力パルス周波数(0 ～ 65535)
//   dcycle  デューティ比 (0～ 4095:4095で100%)
// 戻り値
//   0 正常
//   1 異常(PWMを利用出来ないピンを利用した)
//
uint8_t pwm_out(uint8_t pin, uint16_t freq, uint16_t duty) {
  uint32_t dc;
  timer_dev *dev = PIN_MAP[pin].timer_device;     // ピン対応のタイマーデバイスの取得 
  uint8 cc_channel = PIN_MAP[pin].timer_channel;  // ピン対応のタイマーチャンネルの取得
  if (! (dev && cc_channel) ) 
    return 1;  

  uint32_t f =1000000/(uint32_t)freq;  // 周波数をカウント値に換算
  dc = f*(uint32_t)duty/4095;
  timer_set_prescaler(dev, TIMER_DIV);  // システムクロックを1MHzに分周
  timer_set_reload(dev, f);             // リセットカウント値を設定 
  timer_set_mode(dev, cc_channel,TIMER_PWM);
  timer_set_compare(dev,cc_channel,dc);    // 比較レジスタの初期値指定(デューティ比 0)
  return 0;
}

// PWMコマンド
// PWM ピン番号, DutyCycle, [周波数]
void ipwm() {
  int16_t pinno;      // ピン番号
  int16_t duty;       // デューティー値 0～4095
  int16_t freq = 490; // 周波数
  uint8_t flgok = false;

  if ( getParam(pinno, 0, I_PC15-I_PA0, true) ) return;  // ピン番号取得
  if ( getParam(duty,  0, 4095, false) ) return;         // デューティー値

  if (*cip == I_COMMA) {
    cip++;
    if ( getParam(freq,  0, 32767, false) ) return;      // 周波数の取得
  }

  // PWMピンとして利用可能かチェック
  for (uint8_t i=0; i < sizeof(pwmpins); i++) {
    if (pinno == pwmpins[i]) {
      flgok = true;
      break;
    }
  }  
  if (!flgok) {
    err = ERR_VALUE;
    return;
  }  
 
  if (pwm_out(pinno, freq, duty))
      err = ERR_VALUE; 
}

// shiftOutコマンド SHIFTOUT dataPin, clockPin, bitOrder, value 
void ishiftOut() {
  int16_t dataPin, clockPin;
  int16_t bitOrder;
  int16_t data;

  if (getParam(dataPin, 0,I_PC15-I_PA0, true)) return;
  if (getParam(clockPin,0,I_PC15-I_PA0, true)) return;
  if (getParam(bitOrder,0,1, true)) return;
  if (getParam(data, 0,255,false)) return;

/*
  // データピンの指定
  dataPin = iexp(); if(err) return ; 
  if (dataPin < 0 || dataPin > I_PC15-I_PA0) { err = ERR_VALUE; return; }
  if(*cip != I_COMMA) { err = ERR_SYNTAX; return; }

  // クロックピンの指定
  cip++;
  clockPin = iexp(); if(err) return ; 
  if (clockPin < 0 || clockPin > I_PC15-I_PA0) { err = ERR_VALUE; return; }
  if(*cip != I_COMMA) { err = ERR_SYNTAX; return; }

  // ビットオーダの指定
  cip++;
  bitOrder = iexp(); if(err) return ; 
  if (bitOrder < 0 || bitOrder > 1) { err = ERR_VALUE; return; }
  //if(*cip != I_COMMA) { err = ERR_SYNTAX; return; }

  // データの指定
  cip++;
  data = (uint8_t)iexp(); if(err) return ; 
*/
  
  shiftOut(dataPin, clockPin, bitOrder, data);
}

// 16進文字出力 'HEX$(数値,桁数)' or 'HEX$(数値)'
void ihex(uint8_t devno=0) {
  short value; // 値
  short d = 0; // 桁数(0で桁数指定なし)

  if (checkOpen()) return;
  if (getParam(value,false)) return;  
  if (*cip == I_COMMA) {
     cip++;
     if (getParam(d,0,4,false)) return;  
  }
  if (checkClose()) return;  
  putHexnum(value, d, devno);    
}

// 2進数出力 'BIN$(数値, 桁数)' or 'BIN$(数値)'
void ibin(uint8_t devno=0) {
  int16_t value; // 値
  int16_t d = 0; // 桁数(0で桁数指定なし)

  if (checkOpen()) return;
  if (getParam(value,false)) return;  
  if (*cip == I_COMMA) {
     cip++;
     if (getParam(d,0,16,false)) return;  
  }
  if (checkClose()) return;
  putBinnum(value, d, devno);    
}

// 小数点数値出力 DMP$(数値) or DMP(数値,小数部桁数) or DMP(数値,小数部桁数,整数部桁指定)
void idmp(uint8_t devno=0) {
  int32_t value;     // 値
  int32_t v1,v2;
  int16_t n = 2;    // 小数部桁数
  int16_t dn = 0;   // 整数部桁指定
  int32_t base=1;
  
  if (checkOpen()) return;
  if (getParam(value, false)) return;
  if (*cip == I_COMMA) { 
    cip++; 
    if (getParam(n, 0,4,false)) return;
    if (*cip == I_COMMA) { 
       cip++; 
      if (getParam(dn,-6,6,false)) return;
    }  
  }
  if (checkClose()) return;
  
  for (uint16_t i=0; i<n;i++) {
    base*=10;
  }
  v1 = value / base;
  v2 = value % base;
  if (v1 == 0 && value <0)
    c_putch('-',devno);  
  putnum(v1, dn, devno);
  if (n) {
    c_putch('.',devno);
    putnum(v2<0 ?-v2:v2, -n, devno);
  }
}

// POKEコマンド POKE ADR,データ[,データ,..データ]
void ipoke() {
  uint32_t adr;
  int16_t value;
  
  // アドレスの指定
  value = iexp(); if(err) return ; 
  if (value < 0 ) { err = ERR_VALUE; return; }
  if(*cip != I_COMMA) { err = ERR_SYNTAX; return; }
  adr = SRAM_TOP + value;
    
  // メモリ保護対応
  if ( (adr >= (uint32_t)mem) && (adr < (uint32_t)mem + sizeof(mem)) ||
       (adr >= (uint32_t)var) && (adr < (uint32_t)var + sizeof(var)) ||
       (adr >= (uint32_t)arr) && (adr < (uint32_t)arr + sizeof(arr)) ||
       (adr >= (uint32_t)sc.getScreen()) && (adr < (uint32_t)sc.getScreen()+sc.getScreenByteSize())
   ) {
    // 例: 1,2,3,4,5 の連続設定処理
    do {
      cip++;          // 中間コードポインタを次へ進める
      value = iexp(); // 式の値を取得
      if (err)        // もしエラーが生じたら
        return;       // 終了
  
      *((uint8_t*)adr) = (uint8_t)value;
      adr++;
    } while(*cip == I_COMMA);
  } else {
    err = ERR_RANGE;
  }
}
// I2CW関数  I2CW(I2Cアドレス, コマンドアドレス, コマンドサイズ, データアドレス, データサイズ)
int16_t ii2cw() {
  int16_t  i2cAdr, ctop, clen, top, len;
  uint8_t* ptr;
  uint8_t* cptr;
  int8_t  rc;

  if (checkOpen()) return 0;
  if (getParam(i2cAdr, 0, 0x7f, true)) return 0;
  if (getParam(ctop, 0, 32767, true)) return 0;
  if (getParam(clen, 0, 32767, true)) return 0;
  if (getParam(top, 0, 32767, true)) return 0;
  if (getParam(len, 0, 32767,false)) return 0;
  if (checkClose()) return 0;

  ptr = (uint8_t*)((uint32_t)SRAM_TOP + (uint32_t)top);
  cptr = (uint8_t*)((uint32_t)SRAM_TOP + (uint32_t)ctop);
  if (ctop+clen >= SRAM_SIZE) { err = ERR_RANGE; return 0; }
  if (top+len >= SRAM_SIZE) { err = ERR_RANGE; return 0; }
  
  // I2Cデータ送信
  I2C_WIRE.beginTransmission(i2cAdr);
  if (clen)
    I2C_WIRE.write(cptr, clen);
  if (len)
    I2C_WIRE.write(ptr, len);
  rc =  I2C_WIRE.endTransmission();
  return rc;
}

// I2CR関数  ISND(I2Cアドレス, 送信データアドレス, 送信データサイズ,受信データアドレス,受信データサイズ)
int16_t ii2cr() {
  int16_t  i2cAdr, sdtop, sdlen,rdtop,rdlen;
  uint8_t* sdptr;
  uint8_t* rdptr;
  int8_t  rc;

  if (checkOpen()) return 0;
  if (getParam(i2cAdr, 0, 0x7f, true)) return 0;
  if (getParam(sdtop, 0, 32767, true)) return 0;
  if (getParam(sdlen, 0, 32767, true)) return 0;
  if (getParam(rdtop, 0, 32767, true)) return 0;
  if (getParam(rdlen, 0, 32767,false)) return 0;
  if (checkClose()) return 0;

  sdptr = (uint8_t*)((uint32_t)SRAM_TOP + (uint32_t)sdtop);
  rdptr = (uint8_t*)((uint32_t)SRAM_TOP + (uint32_t)rdtop);
  if (sdtop+sdlen >= SRAM_SIZE) { err = ERR_RANGE; return 0; }
  if (rdtop+rdlen >= SRAM_SIZE) { err = ERR_RANGE; return 0; }
 
  // I2Cデータ送受信
  I2C_WIRE.beginTransmission(i2cAdr);
  
  // 送信
  if (sdlen) {
    I2C_WIRE.write(sdptr, sdlen);
  }

  if (rdlen) {
    rc = I2C_WIRE.endTransmission();
    if (rc!=0)
      return rc;
    I2C_WIRE.requestFrom(i2cAdr, rdlen);
    while (I2C_WIRE.available()) {
      *(rdptr++) = I2C_WIRE.read();
    }
  }  
  rc =  I2C_WIRE.endTransmission();
  return rc;
}

uint8_t _shiftIn( uint8_t ulDataPin, uint8_t ulClockPin, uint8_t ulBitOrder, uint8_t lgc){
  uint8_t value = 0 ;
  uint8_t i ;
  for ( i=0 ; i < 8 ; ++i ) {
    digitalWrite( ulClockPin, lgc ) ;
    if ( ulBitOrder == LSBFIRST )  value |= digitalRead( ulDataPin ) << i ;
    else  value |= digitalRead( ulDataPin ) << (7 - i) ;
    digitalWrite( ulClockPin, !lgc ) ;
  }
  return value ;
}

// SHIFTIN関数 SHIFTIN(データピン, クロックピン, オーダ[,ロジック])
int16_t ishiftIn() {
  int16_t rc;
  int16_t dataPin, clockPin;
  int16_t bitOrder;
  int16_t lgc = HIGH;

  if (checkOpen()) return 0;
  if (getParam(dataPin, 0,I_PC15-I_PA0, true)) return 0;
  if (getParam(clockPin,0,I_PC15-I_PA0, true)) return 0;
  if (getParam(bitOrder,0,1, false)) return 0;
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(lgc,LOW, HIGH, false)) return 0;
  }
  if (checkClose()) return 0;
  rc = _shiftIn((uint8_t)dataPin, (uint8_t)clockPin, (uint8_t)bitOrder, lgc);
  return rc;
}

// SETDATEコマンド  SETDATE 年,月,日,時,分,秒
void isetDate() {
#if USE_INNERRTC == 1
   struct tm t;
  int16_t p_year, p_mon, p_day;
  int16_t p_hour, p_min, p_sec;

  if ( getParam(p_year, 1900,2036, true) ) return; // 年 
  if ( getParam(p_mon,     1,  12, true) ) return; // 月
  if ( getParam(p_day,     1,  31, true) ) return; // 日
  if ( getParam(p_hour,    0,  23, true) ) return; // 時
  if ( getParam(p_min,     0,  59, true) ) return; // 分
  if ( getParam(p_sec,     0,  61, false)) return; // 秒

  // RTCの設定
  t.tm_isdst = 0;             // サーマータイム [1:あり 、0:なし]
  t.tm_year  = p_year - 1900; // 年   [1900からの経過年数]
  t.tm_mon   = p_mon - 1;     // 月   [0-11] 0から始まることに注意
  t.tm_mday  = p_day;         // 日   [1-31]
  t.tm_hour  = p_hour;        // 時   [0-23]
  t.tm_min   = p_min;         // 分   [0-59]  
  t.tm_sec   = p_sec;         // 秒   [0-61] うるう秒考慮
  rtc.setTime(&t);            // 時刻の設定
#else
  err = ERR_SYNTAX; return;
#endif
}

// GETDATEコマンド  SETDATE 年格納変数,月格納変数, 日格納変数, 曜日格納変数
void igetDate() {
#if USE_INNERRTC == 1
  int16_t index;  
  time_t tt; 
  struct tm* st;
  tt = rtc.getTime();   // 時刻取得
  st = localtime(&tt);  // 時刻型変換

  int16_t v[] = {
      st->tm_year+1900, 
      st->tm_mon+1,
      st->tm_mday,
      st->tm_wday
  };

  for (uint8_t i=0; i <4; i++) {    
    if (*cip == I_VAR) {          // 変数の場合
      cip++; index = *cip;        // 変数インデックスの取得
      var[index] = v[i];          // 変数に格納
      cip++;
    } else if (*cip == I_ARRAY) { // 配列の場合      
      cip++;
      index = getparam();         // 添え字の取得
      if (err) return;  
      if (index >= SIZE_ARRY || index < 0 ) {
         err = ERR_SOR;
         return; 
      }
      arr[index] = v[i];          // 配列に格納
    } else {
      err = ERR_SYNTAX;           // 変数・配列でない場合はエラーとする
      return;   
    }     
    if(i != 3) {
      if (*cip != I_COMMA) {      // ','のチェック
         err = ERR_SYNTAX;
         return; 
      }
      cip++;
    }
  }
#else
  err = ERR_SYNTAX;
#endif 
}

// GETDATEコマンド  SETDATE 時格納変数,分格納変数, 秒格納変数
void igetTime() {
#if USE_INNERRTC == 1
  int16_t index;  
  time_t tt; 
  struct tm* st;
  tt = rtc.getTime();   // 時刻取得
  st = localtime(&tt);  // 時刻型変換

  int16_t v[] = {
      st->tm_hour,        // 時
      st->tm_min,         // 分
      st->tm_sec          // 秒
  };

  for (uint8_t i=0; i <3; i++) {    
    if (*cip == I_VAR) {          // 変数の場合
      cip++; index = *cip;        // 変数インデックスの取得
      var[index] = v[i];          // 変数に格納
      cip++;
    } else if (*cip == I_ARRAY) { // 配列の場合      
      cip++;
      index = getparam();         // 添え字の取得
      if (err) return;  
      if (index >= SIZE_ARRY || index < 0 ) {
         err = ERR_SOR;
         return; 
      }
      arr[index] = v[i];          // 配列に格納
    } else {
      err = ERR_SYNTAX;           // 変数・配列でない場合はエラーとする
      return;   
    }     
    if(i != 2) {
      if (*cip != I_COMMA) {      // ','のチェック
         err = ERR_SYNTAX;
         return; 
      }
      cip++;
    }
  }
#else
  err = ERR_SYNTAX;
#endif  
}

// DATEコマンド
void idate() {
#if USE_INNERRTC == 1
   static const char *wday[] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
   time_t tt; 
   struct tm* st;
   tt = rtc.getTime();   // 時刻取得
   st = localtime(&tt);  // 時刻型変換
   
   putnum(st->tm_year+1900, -4);
   c_putch('/');
   putnum(st->tm_mon+1, -2);
   c_putch('/');
   putnum(st->tm_mday, -2);
   c_puts(" [");
   c_puts(wday[st->tm_wday]);
   c_puts("] ");
   putnum(st->tm_hour, -2);
   c_putch(':');
   putnum(st->tm_min, -2);
   c_putch(':');
   putnum(st->tm_sec, -2);
   newline();  
#else
  err = ERR_SYNTAX;
#endif  
 }

// EEPFORMAT コマンド
void ieepformat() {
  uint16_t Status;
  
  //Status = EEPROM.init();  
//  if (Status != EEPROM_OK)
//     EEPROM.erases(&Status);
     Status = EEPROM.format();
  if (Status != EEPROM_OK) {
    switch(Status) {
      case EEPROM_OUT_SIZE:      err = ERR_EEPROM_OUT_SIZE;break;
      case EEPROM_BAD_ADDRESS:   err = ERR_EEPROM_BAD_ADDRESS;break;
      case EEPROM_NOT_INIT:      err = ERR_EEPROM_NOT_INIT;break;
      case EEPROM_NO_VALID_PAGE: err = ERR_EEPROM_NO_VALID_PAGE;break;
      case EEPROM_BAD_FLASH:     
      default:                   err = ERR_EEPROM_BAD_FLASH;break;
    }
  } 
}

// EEPWRITE アドレス,データ コマンド
void ieepwrite() {
  int16_t adr;     // 書込みアドレス
  uint16_t data;   // データ
  uint16_t Status;
  
  if ( getParam(adr, 0, 32767, true) ) return; // アドレス
  if ( getParam(data, false) ) return;         // データ
     
  // データの書込み
  Status = EEPROM.write(adr, data);
  if (Status != EEPROM_OK) {
    switch(Status) {
      case EEPROM_OUT_SIZE:      err = ERR_EEPROM_OUT_SIZE;break;
      case EEPROM_BAD_ADDRESS:   err = ERR_EEPROM_BAD_ADDRESS;break;
      case EEPROM_NOT_INIT:      err = ERR_EEPROM_NOT_INIT;break;
      case EEPROM_NO_VALID_PAGE: err = ERR_EEPROM_NO_VALID_PAGE;break;
      case EEPROM_BAD_FLASH:     
      default:                   err = ERR_EEPROM_BAD_FLASH;break;
    }
  }  
}

// EEPREAD(アドレス) 関数
int16_t ieepread(uint16_t addr) {
  uint16_t Status;
  uint16_t data;

  if (addr < 0 || addr > 32767) {
    err = ERR_VALUE;
    return 0;
 }

  Status = EEPROM.read(addr, &data);  
  if (Status != EEPROM_OK) {
    switch(Status) {
      case EEPROM_OUT_SIZE:
        err = ERR_EEPROM_OUT_SIZE;
        break;
      case EEPROM_BAD_ADDRESS:   
        //err = ERR_EEPROM_BAD_ADDRESS;
        data = 0; // 保存データが無い場合は0を返す
        break;
      case EEPROM_NOT_INIT:
        err = ERR_EEPROM_NOT_INIT;
        break;
      case EEPROM_NO_VALID_PAGE:
        err = ERR_EEPROM_NO_VALID_PAGE;
        break;
      case EEPROM_BAD_FLASH:     
        default:
        err = ERR_EEPROM_BAD_FLASH;
        break;
    }
  }  
  return data;
}

// ドットの描画 PSET X,Y,C
void ipset() {
 int16_t x,y,c;
  if (getParam(x,true)||getParam(y,true)||getParam(c,false)) 
  if (x < 0) x =0;
  if (y < 0) y =0;
  if (x >= sc.getGWidth())  x = sc.getGWidth()-1;
  if (y >= sc.getGHeight()) y = sc.getGHeight()-1;
  if (c < 0 || c > 2) c = 1;
  sc.pset(x,y,c);
}

// 直線の描画 LINE X1,Y1,X2,Y2,C
void iline() {
 int16_t x1,x2,y1,y2,c;
  if (getParam(x1,true)||getParam(y1,true)||getParam(x2,true)||getParam(y2,true)||getParam(c,false)) 
  if (x1 < 0) x1 =0;
  if (y1 < 0) y1 =0;
  if (x2 < 0) x1 =0;
  if (y2 < 0) y1 =0;
  if (x1 >= sc.getGWidth())  x1 = sc.getGWidth()-1;
  if (y1 >= sc.getGHeight()) y1 = sc.getGHeight()-1;
  if (x2 >= sc.getGWidth())  x2 = sc.getGWidth()-1;
  if (y2 >= sc.getGHeight()) y2 = sc.getGHeight()-1;
  if (c < 0 || c > 2) c = 1;
  sc.line(x1, y1, x2, y2, c);
}

// 円の描画 CIRCLE X,Y,R,C,F
void icircle() {
  int16_t x,y,r,c,f;
  if (getParam(x,true)||getParam(y,true)||getParam(r,true)||getParam(c,true)||getParam(f,false)) 
  if (x < 0) x =0;
  if (y < 0) y =0;
  if (x >= sc.getGWidth())  x = sc.getGWidth()-1;
  if (y >= sc.getGHeight()) y = sc.getGHeight()-1;
  if (c < 0 || c > 2) c = 1;
  if (r < 0) r = 1;
  sc.circle(x, y, r, c, f);
}

// 四角の描画 RECT X1,Y1,X2,Y2,C,F
void irect() {
  int16_t x1,y1,x2,y2,c,f;
  if (getParam(x1,true)||getParam(y1,true)||getParam(x2,true)||getParam(y2,true)||getParam(c,true)||getParam(f,false)) 
    return;
  if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= sc.getGWidth() || y2 >= sc.getGHeight())  {
    err = ERR_VALUE;
    return;      
  }
  if (c < 0 || c > 2) c = 1;
  sc.rect(x1, y1, x2-x1+1, y2-y1+1, c, f);
}

// ビットマップの描画 BITMAP 横座標, 縦座標, アドレス, インデックス, 幅, 高さ [,倍率]
void ibitmap() {
  int16_t  x,y,w,h,d = 1;
  int16_t  index;
  int16_t  value;
  uint32_t adr;

  if (getParam(x,true)||getParam(y,true)||getParam(value,true)||getParam(index,true)||getParam(w,true)||getParam(h,false)) 
    return;
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(d,false)) return;
  }
  adr = (uint32_t)SRAM_TOP +(uint32_t)value;
  if (x < 0) x =0;
  if (y < 0) y =0;
  if (x >= sc.getGWidth())  x = sc.getGWidth()-1;
  if (y >= sc.getGHeight()) y = sc.getGHeight()-1;
  if (index < 0) index = 0;
  if (w < 0) w =1;
  if (h < 0) h =1; 
  if (d < 0) d = 1;
  sc.bitmap(x, y, (uint8_t*)adr, index, w, h, d);
}

// キャラクタスクロール CSCROLL X1,Y1,X2,Y2,方向
// 方向 0: 上, 1: 下, 2: 右, 3: 左
void  icscroll() {
  int16_t  x1,y1,x2,y2,d;

  if (getParam(x1,true)||getParam(y1,true)||getParam(x2,true)||getParam(y2,true)||getParam(d,false))
    return;
  if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= sc.getWidth() || y2 >= sc.getHeight())  {
    err = ERR_VALUE;
    return;      
  }
  if (d < 0 || d > 3) d = 0;
  sc.cscroll(x1, y1, x2-x1+1, y2-y1+1, d);
}

// グラフィックスクロール GSCROLL X1,Y1,X2,Y2,方向
void igscroll() {
  int16_t  x1,y1,x2,y2,d;

  if (getParam(x1,true)||getParam(y1,true)||getParam(x2,true)||getParam(y2,true)||getParam(d,false))
    return;
  if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= sc.getGWidth() || y2 >= sc.getGHeight()) {
    err = ERR_VALUE;
    return;      
  }
  if (d < 0 || d > 3) d = 0; 
  sc.gscroll(x1,y1,x2-x1+1, y2-y1+1, d);
}

// GPRINT x,y,..
void igprint() {
  int16_t x,y;
  if ( getParam(x, 0, sc.getGWidth(), true) )  return;
  if ( getParam(y, 0, sc.getGHeight(),true) )  return;
  sc.set_gcursor(x,y);
  iprint(2);
}

// シリアル1バイト出力 : SWRITE データ
void iswrite() {
  int16_t c; 
  if ( getParam(c, false) ) return;
  sc.Serial_write((uint8_t)c);
}

// シリアルモード設定: SMODE MODE [,"通信速度"]
void ismode() {
  int16_t c;
  uint16_t ln;
  uint32_t baud = 0;

  if ( getParam(c, 0, 2, false) ) return;  
  if (c == 1) {
    if(*cip != I_COMMA) {
      err = ERR_SYNTAX;
      return;      
    }
    cip++;
    if (*cip != I_STR) {
      err = ERR_VALUE;
      return;
    }

    cip++;        //中間コードポインタを次へ進める
    ln = *cip++;  //文字数を取得
  
    // 引数のチェック
    for (uint16_t i=0; i < ln; i++) {
       if (*cip >='0' && *cip <= '9') {
          baud = baud*10 + *cip - '0';
       } else {
          err = ERR_VALUE;
          return;
       }
       cip++;
    }
  }
  sc.Serial_mode((uint8_t)c, baud);
}

// シリアル1オープン
void isopen() {
  uint16_t ln;
  uint32_t baud = 0;

  if(lfgSerial1Opened) {
    isclose();
  }

  if (*cip != I_STR) {
    err = ERR_VALUE;
    return;
  }
  
  cip++;        //中間コードポインタを次へ進める
  ln = *cip++;  //文字数を取得

  // 引数のチェック
  for (uint16_t i=0; i < ln; i++) {
     if (*cip >='0' && *cip <= '9') {
        baud = baud*10 + *cip - '0';
     } else {
        err = ERR_VALUE;
        return;
     }
     cip++;
  }
  sc.Serial_open(baud);
  lfgSerial1Opened = true;
}

// シリアル1クローズ
void isclose() {
  delay(500);
  if(lfgSerial1Opened == true)
    //Serial1.end();
    sc.Serial_close();
  lfgSerial1Opened = false;    
}

// TONE 周波数 [,音出し時間]
void itone() {
  int16_t freq;   // 周波数
  int16_t tm = 0; // 音出し時間

  if ( getParam(freq, 0, 32767, false) ) return;
  if(*cip == I_COMMA) {
    cip++;
    if ( getParam(tm, 0, 32767, false) ) return;
  }
  sc.tone(freq, tm);
}

//　NOTONE
void inotone() {
  sc.notone();  
}

// GPEEK(X,Y)関数の処理
int16_t igpeek() {
  short value; // 値
  short x, y;  // 座標
  
  if (checkOpen()) return 0;
  if ( getParam(x,true) || getParam(y,false) ) return 0; 
  if (checkClose()) return 0;
  if (x < 0 || y < 0 || x >= sc.getGWidth()-1 || y >= sc.getGHeight()-1) return 0;
  return sc.gpeek(x,y);  
}

// GINP(X,Y,H,W,C)関数の処理
int16_t iginp() {
  int16_t x,y,w,h,c;

  if (checkOpen())  return 0;
  if ( getParam(x,true)||getParam(y,true)||getParam(w,true)||getParam(h,true)||getParam(c,false) ) return 0; 
  if (checkClose()) return 0;
  if (x < 0 || y < 0 || x >= sc.getGWidth() || y >= sc.getGHeight() || h < 0 || w < 0) return 0;    
  if (x+w >= sc.getGWidth() || y+h >= sc.getGHeight() ) return 0;     
  return sc.ginp(x, y, w, h, c);  
}

// MAP(V,L1,H1,L2,H2)関数の処理
int16_t imap() {
  int32_t value,l1,h1,l2,h2,rc;

  if (checkOpen()) return 0;
  if ( getParam(value,true)||getParam(l1,true)||getParam(h1,true)||getParam(l2,true)||getParam(h2,false) ) 
    return 0;
  if (checkClose()) return 0;
  if (l1 >= h1 || l2 >= h2 || value < l1 || value > h1) {
    err = ERR_VALUE;
    return 0;
  }
  rc = (value-l1)*(h2-l2)/(h1-l1)+l2;
  return rc;  
}

//
// プログラムのロード実行
// LRUN プログラム番号
// 戻り値
//  1:正常 0:異常
//
uint8_t ilrun() {
  int16_t prgno;
  int16_t mode = 0;
  uint32_t flash_adr;
  uint8_t* tmpcip;

 if ( getParam(prgno, 0, 9, false) ) return 0;
  if(*cip == I_COMMA) {
    cip++;
    if ( getParam(mode, 0, 1, false) ) return 0;
  }
  flash_adr = FLASH_START_ADDRESS + FLASH_PAGE_SIZE*(FLASH_PRG_START_PAGE+ prgno*FLASH_PAGE_PAR_PRG);

  // 指定領域に保存されているかチェックする
  if ( *((uint8_t*)flash_adr) == 0xff && *((uint8_t*)flash_adr+1) == 0xff) {
    err = ERR_NOPRG;
    return 0;
  }
 
  // 現在のプログラムの削除とロード
  inew(mode);
  memcpy(listbuf , (uint8_t*)flash_adr, FLASH_PAGE_SIZE*FLASH_PAGE_PAR_PRG);
  cip = listbuf;
  return 1;
}

// エラーメッセージ出力
// 引数: dlfCmd プログラム実行時 false、コマンド実行時 true
void error(uint8_t flgCmd = false) {
  if (err) {  //もし「OK」ではなかったら
    // もしプログラムの実行中なら（cipがリストの中にあり、clpが末尾ではない場合）
    if (cip >= listbuf && cip < listbuf + SIZE_LIST && *clp && !flgCmd) {
      // エラーメッセージを表示
      c_puts(errmsg[err]);       
      c_puts(" in ");
      putnum(getlineno(clp), 0); // 行番号を調べて表示
      newline();

      // リストの該当行を表示
      putnum(getlineno(clp), 0);
      c_puts(" ");
      putlist(clp + 3);          
      newline();
      err = 0;
      return;
    } else {                   // 指示の実行中なら
      c_puts(errmsg[err]);     // エラーメッセージを表示
      newline();               // 改行
      err = 0;                 // エラー番号をクリア
      return;
    }
  } 
  c_puts(errmsg[err]);         //「OK」またはエラーメッセージを表示
  newline();                   // 改行
  err = 0;                     // エラー番号をクリア
}

void DEBUG_info() {
  Serial.print("SYMBOL NUM:");
  Serial.println(I_EOL, DEC);
}

/*
  TOYOSHIKI Tiny BASIC
  The BASIC entry point
*/

void basic() {
  unsigned char len; // 中間コードの長さ
  uint8_t rc;

  // EEPROM(エミュレーション)の利用設定
  EEPROM.PageBase0 = EEPROM_PAGE0;
  EEPROM.PageBase1 = EEPROM_PAGE1;
  EEPROM.PageSize  = FLASH_PAGE_SIZE;
  //EEPROM.init();
  
  // 環境設定
  loadConfig();

  // 実行環境を初期化
  inew();              

  // スクリーン初期設定
  sc.init(SIZE_LINE, CONFIG.KEYBOARD,CONFIG.NTSC);
  
  I2C_WIRE.begin();  // I2C利用開始

  icls();
  char* textline;    // 入力行

  // 起動メッセージ  
  c_puts("TOYOSHIKI TINY BASIC "); //「TOYOSHIKI TINY BASIC」を表示
  newline();                    // 改行
  c_puts(STR_EDITION);          // 版を区別する文字列「EDITION」を表示
  c_puts(" " STR_VARSION);      // バージョンの表示
  newline();                    // 改行
  error();                      //「OK」またはエラーメッセージを表示してエラー番号をクリア

  // プログラム自動起動
  if (CONFIG.STARTPRG >=0  && loadPrg(CONFIG.STARTPRG) == 0) {
    sc.show_curs(0);
    irun();        // RUN命令を実行
    sc.show_curs(1);
    newline();     // 改行
    c_puts("Autorun No.");putnum(CONFIG.STARTPRG,0);c_puts(" stopped.");
    newline();     // 改行
    err = 0; 
  }
  
  // 端末から1行を入力して実行
  while (1) { //無限ループ
    rc = sc.edit();
    if (rc) {
      textline = (char*)sc.getText();
      if (!strlen(textline) ) {
        newline();
        continue;
      }
      if (strlen(textline) >= SIZE_LINE) {
         err = ERR_LONG;
         newline();
         error();
         continue;  
      }
      
      strcpy(lbuf, textline);
      tlimR((char*)lbuf);
      newline();
    } else {
      continue;
    }
    
    // 1行の文字列を中間コードの並びに変換
    len = toktoi(); // 文字列を中間コードに変換して長さを取得
    if (err) {      // もしエラーが発生したら
      error(true);  // エラーメッセージを表示してエラー番号をクリア
      continue;     // 繰り返しの先頭へ戻ってやり直し
    }

    //中間コードの並びがプログラムと判断される場合
    if (*ibuf == I_NUM) { // もし中間コードバッファの先頭が行番号なら
      *ibuf = len;        // 中間コードバッファの先頭を長さに書き換える
      inslist();          // 中間コードの1行をリストへ挿入
      if (err)            // もしエラーが発生したら
        error();          // エラーメッセージを表示してエラー番号をクリア
      continue;           // 繰り返しの先頭へ戻ってやり直し
    }

    // 中間コードの並びが命令と判断される場合
    if (icom())  // 実行する
        error(false); // エラーメッセージを表示してエラー番号をクリア
  } // 無限ループの末尾
}

// システム環境設定のロード
uint8_t loadConfig() {
  int16_t rc;
  uint16_t data;
  CONFIG.NTSC      =  0;
  CONFIG.KEYBOARD  =  0;
  CONFIG.STARTPRG  = -1;

  // NTSC設定の参照
  rc = EEPROM.read(CONFIG_NTSC, &data);
  if (rc == EEPROM_OK) {
    CONFIG.NTSC = data;
  }
  // キーボード設定の参照
  rc = EEPROM.read(CONFIG_KBD, &data);
  if (rc == EEPROM_OK) {
    CONFIG.KEYBOARD = data;  
  }
  // プログラム自動起動設定の参照
  rc = EEPROM.read(CONFIG_PRG, &data);
  if (rc == EEPROM_OK) {
    CONFIG.STARTPRG = data;  
  }
  return 0;
}

// システム環境設定の保存
uint8_t saveConfig() {
  int16_t  rc;
  uint16_t data;
  uint16_t Status;

  rc = EEPROM.count(&data);
  if (rc != EEPROM_OK) {
     ieepformat();
     if (err)
      return -1;
  }

  Status = EEPROM.write(CONFIG_NTSC, (uint16_t)CONFIG.NTSC);
  if (Status != EEPROM_OK) goto ERR_EEPROM;
  Status = EEPROM.write(CONFIG_KBD, (uint16_t)CONFIG.KEYBOARD);
  if (Status != EEPROM_OK) goto ERR_EEPROM;
  Status = EEPROM.write(CONFIG_PRG, (uint16_t)CONFIG.STARTPRG);
  if (Status != EEPROM_OK) goto ERR_EEPROM;
  goto DONE;

ERR_EEPROM:  
    switch(Status) {
      case EEPROM_OUT_SIZE:      err = ERR_EEPROM_OUT_SIZE;break;
      case EEPROM_BAD_ADDRESS:   err = ERR_EEPROM_BAD_ADDRESS;break;
      case EEPROM_NOT_INIT:      err = ERR_EEPROM_NOT_INIT;break;
      case EEPROM_NO_VALID_PAGE: err = ERR_EEPROM_NO_VALID_PAGE;break;
      case EEPROM_OK: err = 0;break;
      case EEPROM_BAD_FLASH:     
      default:                   err = ERR_EEPROM_BAD_FLASH;break;
    }
    return -1;

DONE:  
  return 0;
}


