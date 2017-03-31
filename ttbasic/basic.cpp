/*
 TOYOSHIKI Tiny BASIC for Arduino
 (C)2012 Tetsuya Suzuki
 GNU General Public License
 */

//
// 2017/03/22 修正, Arduino STM32、フルスクリーン対応, by たま吉さん
// 2017/03/25 修正, INKEY()関数の追加, 中断キーを[ESC]から[CTRL-C]に変更
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
// 12)追加: 論理積、論理和 '|'、'&'演算子の追加
// 13)追加: MILLIS() 起動から現在までの時間の取得
// 

#include <Arduino.h>
#include <stdlib.h>
#include "tscreen.h"

// TOYOSHIKI TinyBASIC symbols
// TO-DO Rewrite defined values to fit your machine as needed
#define SIZE_LINE 80 //Command line buffer length + NULL
#define SIZE_IBUF 80 //i-code conversion buffer size
#define SIZE_LIST 2048 //List buffer size
#define SIZE_ARRY 32 //Array area size
#define SIZE_GSTK 6 //GOSUB stack size(2/nest)
#define SIZE_LSTK 15 //FOR stack size(5/nest)

// Depending on device functions
// TO-DO Rewrite these functions to fit your machine
#define STR_EDITION "ARDUINO STM32"

// Terminal control
#define c_getch( ) sc.get_ch()
#define c_kbhit( ) sc.isKeyIn()
#define c_putch(c) sc.putch(c)

// 定数
#define CONST_HIGH   1
#define CONST_LOW    0
#define CONST_LSB    LSBFIRST
#define CONST_MSB    MSBFIRST

tscreen sc; // スクリーン制御

// 追加コマンドの宣言
void icls();
void iwait(uint16_t tm);
void ilocate();
void icolor();
void iattr();
int16_t iinkey();
int16_t ivpeek();
void ipmode();
void idwrite();
void ihex();
void ibin();
void ishiftOut();
      
#define KEY_ENTER 13
void newline(void) {
/*
  c_putch(13); //CR
  c_putch(10); //LF
*/
 sc.newLine();
}

// Return random number
short getrnd(short value) {
  return random(value) + 1;
}

// Prototypes (necessity minimum)
short iexp(void);

// ピンモード
const WiringPinMode pinType[] = {
  OUTPUT_OPEN_DRAIN, OUTPUT, INPUT_PULLUP, INPUT_PULLDOWN, INPUT_ANALOG, INPUT, 
};

// Keyword table
const char *kwtbl[] = {
  "GOTO", "GOSUB", "RETURN",
  "FOR", "TO", "STEP", "NEXT",
  "IF", "REM", "END",
  "PRINT", "LET",
  "CLS", "WAIT", "LOCATE", "COLOR", "ATTR" ,"INKEY", "?", "VPEEK", "CHR$" , "ASC", "RENUM", "HEX$", "BIN$", 
  "HIGH", "LOW",
  ",", ";", ":", "\'",
  "-", "+", "*", "/", "%", "(", ")", "$", "<<", ">>", "|", "&", 
  ">=", "#", ">", "=", "<=", "<>", "<", 
  "@", "RND", "ABS", "FREE", "TICK", 

  "OUTPUT_OD", "OUTPUT", "INPUT_PU", "INPUT_PD", "ANALOG", "INPUT_FL",
  "INPUT", "GPIO", "OUT", "IN", "ANA", "SHIFTOUT", 
  "PA00", "PA01", "PA02", "PA03", "PA04", "PA05", "PA06", "PA07", "PA08", 
  "PA09", "PA10", "PA11", "PA12", "PA13","PA14","PA15",
  "PB00", "PB01", "PB02", "PB03", "PB04", "PB05", "PB06", "PB07", "PB08", 
  "PB09", "PB10", "PB11", "PB12", "PB13","PB14","PB15",
  "PC13", "PC14","PC15", 
  "LSB", "MSB",
  "LIST", "RUN", "NEW", "OK",
};

// Keyword count
#define SIZE_KWTBL (sizeof(kwtbl) / sizeof(const char*))

// i-code(Intermediate code) assignment
enum {
  I_GOTO, I_GOSUB, I_RETURN,
  I_FOR, I_TO, I_STEP, I_NEXT,
  I_IF, I_REM, I_END,
  I_PRINT, I_LET,
  I_CLS, I_WAIT, I_LOCATE, I_COLOR, I_ATTR, I_INKEY, I_QUEST, I_VPEEK, I_CHR, I_ASC, I_RENUM, I_HEX, I_BIN,
  I_HIGH, I_LOW, 
  I_COMMA, I_SEMI, I_COLON, I_SQUOT,
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_DOLLAR, I_LSHIFT, I_RSHIFT, I_LOR, I_LAND,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_LT, 
  I_ARRAY, I_RND, I_ABS, I_FREE, I_TICK, 
  
  I_OUTPUT_OPEN_DRAIN, I_OUTPUT, I_INPUT_PULLUP, I_INPUT_PULLDOWN, I_INPUT_ANALOG, I_INPUT_F,
  
  I_INPUT, I_GPIO, I_DOUT, I_DIN, I_ANA, I_SHIFTOUT, 
  
  I_PA0, I_PA1, I_PA2, I_PA3, I_PA4, I_PA5, I_PA6, I_PA7, I_PA8, 
  I_PA9, I_PA10, I_PA11, I_PA12, I_PA13,I_PA14,I_PA15,
  I_PB0, I_PB1, I_PB2, I_PB3, I_PB4, I_PB5, I_PB6, I_PB7, I_PB8, 
  I_PB9, I_PB10, I_PB11, I_PB12, I_PB13,I_PB14,I_PB15,
  I_PC13, I_PC14,I_PC15,
  I_LSB, I_MSB,
  I_LIST, I_RUN, I_NEW, I_OK,
  I_NUM, I_VAR, I_STR, I_HEXNUM, 
  I_EOL
};

// List formatting condition
// 後ろに空白を入れない中間コード
const unsigned char i_nsa[] = {
  I_RETURN, I_END, 
  I_CLS,
  I_HIGH, I_LOW, 
  I_INKEY,I_VPEEK, I_CHR, I_ASC, I_HEX, I_BIN, 
  I_COMMA, I_SEMI, I_COLON, I_SQUOT,
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_DOLLAR, I_LSHIFT, I_RSHIFT, I_LOR, I_LAND,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_LT,  
  I_ARRAY, I_RND, I_ABS, I_FREE, I_TICK,
  I_OUTPUT_OPEN_DRAIN, I_OUTPUT, I_INPUT_PULLUP, I_INPUT_PULLDOWN, I_INPUT_ANALOG, I_INPUT_F,
  I_DIN, I_ANA, 
  I_PA0, I_PA1, I_PA2, I_PA3, I_PA4, I_PA5, I_PA6, I_PA7, I_PA8, 
  I_PA9, I_PA10, I_PA11, I_PA12, I_PA13,I_PA14,I_PA15,
  I_PB0, I_PB1, I_PB2, I_PB3, I_PB4, I_PB5, I_PB6, I_PB7, I_PB8, 
  I_PB9, I_PB10, I_PB11, I_PB12, I_PB13,I_PB14,I_PB15,
  I_PC13, I_PC14,I_PC15,
  I_LSB, I_MSB,
};

// 前が定数か変数のとき前の空白をなくす中間コード
const unsigned char i_nsb[] = {
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_LSHIFT, I_RSHIFT, I_LOR, I_LAND,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_LT, 
  I_COMMA, I_SEMI, I_COLON, I_SQUOT, I_EOL
};

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
  "Illegal value", // 追加
  "Syntax error",
  "Internal error",
  "Abort by [CTRL-C] or [ESC]"
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
  ERR_SYNTAX,
  ERR_SYS,
  ERR_CTR_C
};

// RAM mapping
char lbuf[SIZE_LINE];             // コマンド入力バッファ
unsigned char ibuf[SIZE_IBUF];    // i-code conversion buffer
short var[26];                    // 変数領域
short arr[SIZE_ARRY];             // 配列領域
unsigned char listbuf[SIZE_LIST]; // プログラムリスト領域
unsigned char* clp;               // Pointer current line
unsigned char* cip;               // Pointer current Intermediate code
unsigned char* gstk[SIZE_GSTK];   // GOSUB stack
unsigned char gstki;              // GOSUB stack index
unsigned char* lstk[SIZE_LSTK];   // FOR stack
unsigned char lstki;              // FOR stack index

uint8_t prevPressKey = 0;         // 直前入力キーの値(INKEY()、[ESC]中断キー競合防止用)

// Standard C libraly (about) same functions
char c_toupper(char c) {
  return(c <= 'z' && c >= 'a' ? c - 32 : c);
}
char c_isprint(char c) {
  return(c >= 32 && c <= 126);
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

void c_puts(const char *s) {
  while (*s) c_putch(*s++); //終端でなければ出力して繰り返す
}

// Print numeric specified columns
// 引数
//  value : 出力対象数値
//  d     : 桁指定(0で指定無し)
// 機能
// 'SNNNNN' S:符号 N:数値 or 空白 
//  dで桁指定時は空白補完する
//
void putnum(short value, short d) {
  unsigned char dig;  // 桁位置
  unsigned char sign; // 負号の有無（値を絶対値に変換した印）

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
    c_putch(' ');       // 桁の不足を空白で埋める
    d--;                // 指定の桁数を1減らす
  }
  c_puts(&lbuf[dig]);   // 桁位置からバッファの文字列を表示
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
void putHexnum(short value, uint8_t d) {
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
  c_puts(&lbuf[4-dig]);
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
void putBinnum(short value, uint8_t d) {
  uint16_t  bin = (uint16_t)value; // 符号なし16進数として参照利用する
  uint16_t  b;
  uint16_t  dig = 0;
  
  for (uint8_t i = 0; i < 16; i++) {
    b = bin>>(15-i);
    lbuf[i] = b ? '1':'0';
    if (!dig && b) 
      dig = 16-i;
  }
  lbuf[16] = 0;

  if (d > dig)
    dig = d;
  c_puts(&lbuf[16-dig]);
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
      if (!nospacea(*ip)) //もし例外にあたらなければ
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
      if (*ip == I_VAR) //もし次の中間コードが変数だったら
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
short ivalue() {
  short value; // 値
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

  // 定数HIGH/LOWの場合
  case I_HIGH: 
   cip++; value = CONST_HIGH;  break;
  case I_LOW:
   cip++;  value = CONST_LOW;  break;

  case I_LSB:
   cip++; value = CONST_LSB;  break;
  case I_MSB:
   cip++; value = CONST_MSB;  break;
  
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
  
  default: //以上のいずれにも該当しなかった場合
    // 定数ピン番号
    if (*cip >= I_PA0 && *cip <= I_PC15) {
      value = *cip - I_PA0; 
      cip++;
      return value;
    // 定数GPIOモード
    } else  if (*cip >= I_OUTPUT_OPEN_DRAIN && *cip <= I_INPUT_F) {
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

   case I_LAND:  // 論理積(ビット演算)
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)&((uint16_t)tmp);
    break; //ここで打ち切る

   case I_LOR:   //論理和(ビット演算)
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)|((uint16_t)tmp);
    break; //ここで打ち切る

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

  default: //以上のいずれにも該当しなかった場合
    return value; //値を持ち帰る
  } //中間コードで分岐の末尾
}

// PRINT handler
void iprint() {
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
        c_putch(*cip++); //文字を表示
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
     if(!sc.IS_PRINT(value))
        value = 32;
     c_putch(value);
      break; // 打ち切る

    case I_HEX: // HEX$()関数の場合
      cip++;    // 中間コードポインタを次へ進める
      ihex();
      break; //打ち切る

    case I_BIN: // BIN$()関数の場合
      cip++;    // 中間コードポインタを次へ進める
      ibin();
      break; //打ち切る

    default: //以上のいずれにも該当しなかった場合（式とみなす）
      value = iexp(); //値を取得
      if (err) { //もしエラーが生じたら
        newline(); //改行        
        return; //終了
      }
      putnum(value, len); //値を表示
      break; //打ち切る
    } //中間コードで分岐の末尾

    if (*cip == I_COMMA|*cip == I_SEMI) { //もしコンマがあったら
      cip++; //中間コードポインタを次へ進める
      if (*cip == I_COLON || *cip == I_EOL) //もし文末なら
        //newline(); //改行        
        return; //終了
    } else { //コンマがなければ
      if (*cip != I_COLON && *cip != I_EOL) { //もし文末でなければ
        err = ERR_SYNTAX; //エラー番号をセット
        newline(); //改行        
        return; //終了
      }
    }
  } //文末まで繰り返すの末尾

  newline(); //改行
}

// INPUT handler
void iinput() {
  short value;          // 値
  short index;          // 配列の添え字
  unsigned char i;      // 文字数
  unsigned char prompt; //プロンプト表示フラグ

  while (1) {           // 無限に繰り返す
    prompt = 1;         // まだプロンプトを表示していない

    // プロンプトが指定された場合の処理
    if(*cip == I_STR){   // もし中間コードが文字列なら
      cip++;             // 中間コードポインタを次へ進める
      i = *cip++;        // 文字数を取得
      while (i--)        // 文字数だけ繰り返す
        c_putch(*cip++); // 文字を表示
      prompt = 0;        // プロンプトを表示した
    }

    // 値を入力する処理
    switch (*cip) {         // 中間コードで分岐
    case I_VAR:              // 変数の場合
      cip++;                 // 中間コードポインタを次へ進める
      if (prompt) {          // もしまだプロンプトを表示していなければ
        c_putch(*cip + 'A'); // 変数名を表示
        c_putch(':');        //「:」を表示
      }
      value = getnum();    // 値を入力
      if (err)             // もしエラーが生じたら
        return;            // 終了
      var[*cip++] = value; // 変数へ代入
      break;               // 打ち切る

    case I_ARRAY: // 配列の場合
      cip++;               // 中間コードポインタを次へ進める
      index = getparam();  // 配列の添え字を取得
      if (err)             // もしエラーが生じたら
        return;            // 終了
      if (index >= SIZE_ARRY) { // もし添え字が上限を超えたら
        err = ERR_SOR;          // エラー番号をセット
        return;                 // 終了
      }
      if (prompt) { // もしまだプロンプトを表示していなければ
        c_puts("@(");     //「@(」を表示
        putnum(index, 0); // 添え字を表示
        c_puts("):");     //「):」を表示
      }
      value = getnum(); // 値を入力
      if (err)            // もしエラーが生じたら
        return;           // 終了
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

// Execute a series of i-code
unsigned char* iexe() {
  short lineno;            // 行番号
  unsigned char* lp;       // 未確定の（エラーかもしれない）行ポインタ
  short index, vto, vstep; // FOR文の変数番号、終了値、増分
  short condition;         // IF文の条件値
  uint16_t prm;            // コマンド引数
  uint8_t c;               // 入力キー
  
  while (*cip != I_EOL) { //行末まで繰り返す
  
  //強制的な中断の判定
  if (c_kbhit()) { // もし未読文字があったら
      c = c_getch();
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
      if (condition) // もし真なら
        break;       // 打ち切る（次の文を実行する）
      //偽の場合の処理はREMと同じ

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
      if(prm < 0) prm = 0;
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
      iinput();   // INPUT文を実行
      break;

    case I_GPIO:  // GPIO
      cip++;
      ipmode();
      break;    
      
    case I_DOUT: // OUT
      cip++;
      idwrite();
      break;    

    case I_SHIFTOUT: // ShiftOut
      cip++;
      ishiftOut();
      break;    
    
    
    case I_NEW:   // 中間コードがNEWの場合
    case I_LIST:  // 中間コードがLISTの場合
    case I_RUN:   // 中間コードがRUNの場合
    case I_RENUM: // 中間コードがRENUMの場合
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
  return clp + *clp; //次に実行するべき行のポインタを持ち帰る
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

// LIST command handler
void ilist() {
  short lineno; //表示開始行番号

  //表示開始行番号の設定
  if (*cip == I_NUM) //もしLIST命令に引数があったら
    lineno = getlineno(cip); //引数を読み取って表示開始行番号とする
  else //引数がなければ
    lineno = 0; //表示開始行番号を0とする

  //行ポインタを表示開始行番号へ進める
  for ( //次の手順で繰り返す
    clp = listbuf; //行ポインタを先頭行へ設定
    //末尾ではなくて表示開始行より前なら繰り返す
    *clp && (getlineno(clp) < lineno);
    clp += *clp); //行ポインタを次の行へ進める

  //リストを表示する
  while (*clp) { //行ポインタが末尾を指すまで繰り返す
    putnum(getlineno(clp), 0); //行番号を表示
    c_putch(' '); //空白を入れる
    putlist(clp + 3); //行番号より後ろを文字列に変換して表示
    if (err) //もしエラーが生じたら
      break; //繰り返しを打ち切る
    newline(); //改行
    clp += *clp; //行ポインタを次の行へ進める
  }
}

//NEW command handler
void inew(void) {
  unsigned char i; //ループカウンタ

  //変数と配列の初期化
  for (i = 0; i < 26; i++) //変数の数だけ繰り返す
    var[i] = 0; //変数を0に初期化
  for (i = 0; i < SIZE_ARRY; i++) //配列の数だけ繰り返す
    arr[i] = 0; //配列を0に初期化

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
  if (startLineNo <= 0 || increase <= 0 || increase >=1000) {
	  err = ERR_SOR;
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

//Command precessor
uint8_t icom() {
  uint8_t rc = 1;
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
    if (*cip == I_EOL || //もし行末か、あるいは
      *(cip + 3) == I_EOL) //続いて引数があれば
      ilist();             // LIST命令を実行
    else //そうでなければ
      err = ERR_SYNTAX; //エラー番号をセット
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

  if ( fc > 9 )  fc = 9;         // 文字色範囲チェック
  else if ( fc < 0)  fc = 0;
  
  if( bc > 9 ) bc = 9;          // 背景色範囲ッチェック
  else if( bc < 0)  bc = 0;
  // 文字色の設定
  sc.setColor((uint16_t)fc, (uint16_t)bc);  
}

// 文字属性の指定 ATTRコマンド
void iattr() {
 int16_t attr;

  attr = iexp(); if(err) return ; // 文字属性の取得

  if ( attr > 5 )  attr = 5;      // 有効性のチェク
  else if ( attr < 0)  attr = 0;
  sc.setAttr(attr);               // 文字属性のセット
}

// キー入力文字コードの取得 INKEY()関数
int16_t iinkey() {
  int16_t rc = 0;
  
  if (prevPressKey) {
    // 一時バッファに入力済キーがあればそれを使う
    rc = prevPressKey;
    prevPressKey = 0;
  } else if (sc.isKeyIn()) {
    // キー入力
    rc = sc.get_ch();
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
void ipmode() {
  int16_t pinno;       // ピン番号
  WiringPinMode pmode; // 入出力モード

  // 入出力ピンの指定
  pinno = iexp(); if(err) return ;                                     // ピン番号取得
  if (pinno < 0 || pinno > I_PC15-I_PA0) { err = ERR_VALUE; return; } // 範囲チェック
  if(*cip != I_COMMA) { err = ERR_SYNTAX; return; }                    // ','のチャック
 
  // 入出力モードの指定
  cip++;  pmode = (WiringPinMode)iexp();  if(err) return ;             // 入出力モードの取得
  
  // ピンモードの設定
  pinMode(pinno, pmode);
}

// GPIO ピンデジタル出力
void idwrite() {
  int16_t pinno,  data;

  // 出力ピンの指定
  pinno = iexp(); if(err) return ; 
  if (pinno < 0 || pinno > I_PC15-I_PA0) { err = ERR_VALUE; return; }
  if(*cip != I_COMMA) { err = ERR_SYNTAX; return; }
 
  //データの指定
  cip++;
  data = iexp();  if(err) return ;
  data = data ? HIGH: LOW;
  
  // ピンモードの設定
  digitalWrite(pinno, data);
}

// shiftOutコマンド SHIFTOUT dataPin, clockPin, bitOrder, value 
void ishiftOut() {
  int16_t dataPin, clockPin;
  int16_t bitOrder;
  uint8_t data;

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
  if(*cip != I_COMMA) { err = ERR_SYNTAX; return; }

  // データの指定
  cip++;
  data = (uint8_t)iexp(); if(err) return ; 
  shiftOut(dataPin, clockPin, bitOrder, data);
}

// 16進文字出力 'HEX$(数値,桁数)' or 'HEX$(数値)'
void ihex() {
  short value; // 値
  short d = 0; // 桁数(0で桁数指定なし)
  if (*cip != I_OPEN)  { err = ERR_PAREN; return; }  // '('のチェック
  cip++; value = iexp(); if (err) return;            // 値の取得
  if (*cip == I_COMMA) {                            
      cip++; d = iexp();if (err) return;             // 桁数の取得
  }
  if (*cip != I_CLOSE) { err = ERR_PAREN;  return; } // ')'のチェック
  cip++; 

   // 桁数指定の有効性チェック
  if (d < 0 || d > 4) {
    err = ERR_VALUE;
    return;
  }
  putHexnum(value, d);    
}

// 2進数出力 'BIN$(数値, 桁数)' or 'BIN$(数値)'
void ibin() {
  short value; // 値
  short d = 0; // 桁数(0で桁数指定なし)
  if (*cip != I_OPEN)  { err = ERR_PAREN; return; }  // '('のチェック
  cip++; value = iexp(); if (err) return;            // 値の取得
  if (*cip == I_COMMA) {                            
      cip++; d = iexp();if (err) return;             // 桁数の取得
  }
  if (*cip != I_CLOSE) { err = ERR_PAREN;  return; } // ')'のチェック
  cip++; 

   // 桁数指定の有効性チェック
  if (d < 0 || d > 16) {
    err = ERR_VALUE;
    return;
  }
  putBinnum(value, d);    
  
}
// Print OK or error message
void error() {
  if (err) {                   //もし「OK」ではなかったら
    // もしプログラムの実行中なら（cipがリストの中にあり、clpが末尾ではない場合）
    if (cip >= listbuf && cip < listbuf + SIZE_LIST && *clp) {
      newline();                 // 改行
      c_puts("LINE:");           //「LINE:」を表示
      putnum(getlineno(clp), 0); // 行番号を調べて表示
      c_putch(' ');              // 空白を表示
      putlist(clp + 3);          // リストの該当行を表示
      newline();                 // 改行
    } else {                     // 指示の実行中なら
      //newline();               // 改行
      c_puts("YOU TYPE: ");      //「YOU TYPE:」を表示
      c_puts(lbuf);              // 文字列バッファの内容を表示
      newline();                 // 改行
    }
  } //もし「OK」ではなかったらの末尾

  //newline(); //改行
  c_puts(errmsg[err]);           //「OK」またはエラーメッセージを表示
  newline();                     // 改行
  err = 0;                       // エラー番号をクリア
}

/*
  TOYOSHIKI Tiny BASIC
  The BASIC entry point
*/

void basic() {
  unsigned char len; // 中間コードの長さ
  uint8_t rc;
  
  inew();            // 実行環境を初期化
  sc.init(80,22,80); // スクリーン初期設定
  sc.cls();
  
  // 起動メッセージ
  c_puts("TOYOSHIKI TINY BASIC"); //「TOYOSHIKI TINY BASIC」を表示
  newline();                      // 改行
  c_puts(STR_EDITION);            // 版を区別する文字列を表示
  c_puts(" EDITION");             //「 EDITION」を表示
  newline();                      // 改行
  error();                        //「OK」またはエラーメッセージを表示してエラー番号をクリア

  // 端末から1行を入力して実行
  while (1) { //無限ループ
    rc = sc.edit();
    if (rc) {
      if (!strlen((char*)sc.getText()) ) {
        newline();
        continue;
      }
      strcpy(lbuf, (char*)sc.getText());
      newline();
    } else {
      continue;
    }
    
    // 1行の文字列を中間コードの並びに変換
    len = toktoi(); // 文字列を中間コードに変換して長さを取得
    if (err) {      // もしエラーが発生したら
      error();      // エラーメッセージを表示してエラー番号をクリア
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
        error(); // エラーメッセージを表示してエラー番号をクリア
  } // 無限ループの末尾
}
