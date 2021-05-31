//
// PS/2キー入力をtty入力コードにコンバートする
// 作成 2017/04/09
// 修正 2017/04/15, 利用ピンの変更(I2Cとの競合のため)
// 修正 2017/04/29, キーボード、NTSCの補正対応
// 修正 2017/05/09, カナ入力対応
// 修正 2017/05/18, ltu(ﾂ)が利用出来ない不具合の対応

#include <string.h>
#include <TKeyboard.h>
#include "tTVscreen.h"

#ifndef __DJGPP__  // DOS uses Allegro keyboard system

#if (defined(ESP8266) || defined(ESP32)) && !defined(__FLASH__)
#define __FLASH__ PROGMEM
#endif

#ifndef ESP8266_NOWIFI
// workaround until there is a basic_engine board in mainline Arduino
#ifdef ESP32
// less than 32, or checkKbd() has to be updated
static const uint8_t PS2CLK = 14;
static const uint8_t PS2DAT = 13;
#else
static const uint8_t PS2CLK = 4;
static const uint8_t PS2DAT = 5;
#endif
#endif

#include "ps22tty.h"

const int IRQpin = PS2CLK;   // CLK(D+)
const int DataPin = PS2DAT;  // Data(D-)

TKeyboard kb;             // PS/2キーボードドライバ
uint8_t flgKana = 0;  // カナ入力モード
struct ring_buffer kbuf;  // キーバッファ
uint16_t kdata[16];

#define RK_ENTRY_NUM (sizeof(RomaKatakana) / sizeof(RomaKatakana[0]))

//子音状態遷移コード
// clang-format off
enum {
  _romaji_top = 0,
  _romaji_b, _romaji_c, _romaji_d, _romaji_f, _romaji_g,  _romaji_h, _romaji_j, _romaji_k, _romaji_l, _romaji_m,
  _romaji_n, _romaji_p, _romaji_q, _romaji_r, _romaji_s,  _romaji_t, _romaji_v, _romaji_w, _romaji_x, _romaji_y,
  _romaji_z, _romaji_by,_romaji_ch,_romaji_cy,_romaji_dh, _romaji_dw,_romaji_dy,_romaji_fw,_romaji_fy,_romaji_gw,
  _romaji_gy,_romaji_hy,_romaji_jy,_romaji_kw,_romaji_ky, _romaji_lt,_romaji_ly,_romaji_my,_romaji_ny,_romaji_py,
  _romaji_qw,_romaji_qy,_romaji_ry,_romaji_sh,_romaji_sw,_romaji_sy, _romaji_th,_romaji_ts,_romaji_tw,_romaji_ty,
  _romaji_vy,_romaji_wh,_romaji_xt,_romaji_xy,_romaji_zy,
};

// カタカタ文字列変換テーブル
const wchar_t RomaKatakana[][5][3] PROGMEM =  {
  //  a      e      i      o      u
  { L"ア", L"エ", L"イ", L"オ", L"ウ", },               //[]  : ｱ ｴ ｲ ｵ ｳ
  { L"バ", L"ベ", L"ビ", L"ボ", L"ブ", },               //[b] : ﾊﾞ ﾍﾞ ﾋﾞ ﾎﾞ ﾌﾞ
  { L"カ", L"セ", L"シ", L"コ", L"ク", },               //[c] : ｶ ｾ ｼ ｺ ｸ
  { L"ダ", L"デ", L"ヂ", L"ド", L"ヅ", },               //[d] : ﾀﾞ ﾃﾞ ﾁﾞ ﾄﾞ ﾂﾞ
  { L"ファ", L"フェ", L"フィ", L"フォ", L"フ", },       //[f] : ﾌｧ ﾌｪ ﾌｨ ﾌｫ ﾌ
  { L"ガ", L"ゲ", L"ギ", L"ゴ", L"グ", },               //[g] : ｶﾞ ｹﾞ ｷﾞ ｺﾞ ｸﾞ
  { L"ハ", L"ヘ", L"ヒ", L"ホ", L"フ", },               //[h] : ﾊ ﾍ ﾋ ﾎ ﾌ
  { L"ジャ", L"ジェ", L"ジ", L"ジョ", L"ジュ", },       //[j] : ｼﾞｬ ｼﾞｪ ｼﾞ ｼﾞｮ ｼﾞｭ
  { L"カ", L"ケ", L"キ", L"コ", L"ク", },               //[k] : ｶ ｹ ｷ ｺ ｸ
  { L"ァ", L"ェ", L"ィ", L"ォ", L"ゥ", },               //[l] : ｧ ｪ ｨ ｫ ｩ
  { L"マ", L"メ", L"ミ", L"モ", L"ム", },               //[m] : ﾏ ﾒ ﾐ ﾓ ﾑ
  { L"ナ", L"ネ", L"ニ", L"ノ", L"ヌ", },               //[n] : ﾅ ﾈ ﾆ ﾉ ﾇ
  { L"パ", L"ペ", L"ピ", L"ポ", L"プ", },               //[p] : ﾊﾟ ﾍﾟ ﾋﾟ ﾎﾟ ﾌﾟ
  { L"クァ", L"クェ", L"クィ", L"クォ", L"ク", },       //[q] : ｸｧ ｸｪ ｸｨ ｸｫ ｸ
  { L"ラ", L"レ", L"リ", L"ロ", L"ル", },               //[r] : ﾗ ﾚ ﾘ ﾛ ﾙ
  { L"サ", L"セ", L"シ", L"ソ", L"ス", },               //[s] : ｻ ｾ ｼ ｿ ｽ
  { L"タ", L"テ", L"チ", L"ト", L"ツ", },               //[t] : ﾀ ﾃ ﾁ ﾄ ﾂ
  { L"ヴァ", L"ヴェ", L"ヴィ", L"ヴォ", L"ヴ", },       //[v] : ｳﾞｧ ｳﾞｪ ｳﾞｨ ｳﾞｫ ｳﾞ
  { L"ワ", L"ウェ", L"ウィ", L"ヲ", L"ウ", },           //[w] : ﾜ ｳｪ ｳｨ ｦ ｳ
  { L"ァ", L"ェ", L"ィ", L"ォ", L"ゥ", },               //[x] : ｧ ｪ ｨ ｫ ｩ
  { L"ヤ", L"ィェ", L"イ", L"ヨ", L"ユ", },             //[y] : ﾔ ｨｪ ｲ ﾖ ﾕ
  { L"ザ", L"ゼ", L"ジ", L"ゾ", L"ズ", },               //[z] : ｻﾞ ｾﾞ ｼﾞ ｿﾞ ｽﾞ
  { L"ビャ", L"ビェ", L"ビィ", L"ビョ", L"ビュ"},       //[by] : ﾋﾞｬ ﾋﾞｪ ﾋﾞｨ ﾋﾞｮ ﾋﾞｭ
  { L"チャ", L"チェ", L"チ", L"チョ", L"チュ", },       //[ch] : ﾁｬ ﾁｪ ﾁ ﾁｮ ﾁｭ
  { L"チャ", L"チェ", L"チィ", L"チョ", L"チュ", },     //[cy] : ﾁｬ ﾁｪ ﾁｨ ﾁｮ ﾁｭ
  { L"デャ", L"デェ", L"ディ", L"デョ", L"デュ", },     //[dh] : ﾃﾞｬ ﾃﾞｪ ﾃﾞｨ ﾃﾞｮ ﾃﾞｭ
  { L"ドァ", L"ドェ", L"ドィ", L"ドォ", L"ドゥ", },     //[dw] : ﾄﾞｧ ﾄﾞｪ ﾄﾞｨ ﾄﾞｫ ﾄﾞｩ
  { L"ヂャ", L"ヂェ", L"ヂィ", L"ヂョ", L"ヂュ", },     //[dy] : ﾁﾞｬ ﾁﾞｪ ﾁﾞｨ ﾁﾞｮ ﾁﾞｭ
  { L"ファ", L"フェ", L"フィ", L"フォ", L"フゥ", },     //[fw] : ﾌｧ ﾌｪ ﾌｨ ﾌｫ ﾌｩ
  { L"フャ", L"フェ", L"フィ", L"フョ", L"フュ", },     //[fy] : ﾌｬ ﾌｪ ﾌｨ ﾌｮ ﾌｭ
  { L"グァ", L"グェ", L"グィ", L"グォ", L"グゥ", },     //[gw] : ｸﾞｧ ｸﾞｪ ｸﾞｨ ｸﾞｫ ｸﾞｩ
  { L"ギャ", L"ギェ", L"ギィ", L"ギョ", L"ギュ", },     //[gy] : ｷﾞｬ ｷﾞｪ ｷﾞｨ ｷﾞｮ ｷﾞｭ
  { L"ヒャ", L"ヒェ", L"ヒィ", L"ヒョ", L"ヒュ", },     //[hy] : ﾋｬ ﾋｪ ﾋｨ ﾋｮ ﾋｭ
  { L"ジャ", L"ジェ", L"ジィ", L"ジョ", L"ジュ", },     //[jy] : ｼﾞｬ ｼﾞｪ ｼﾞｨ ｼﾞｮ ｼﾞｭ
  { L"クァ", L"", L"", L"", L"", },                     //[kw] : ｸｧ NG NG NG NG
  { L"キャ", L"キェ", L"キィ", L"キョ", L"キュ", },     //[ky] : ｷｬ ｷｪ ｷｨ ｷｮ ｷｭ
  { L"", L"", L"", L"", L"ッ", },                       //[lt] : NG NG NG NG ｯ
  { L"ャ", L"ェ", L"ィ", L"ョ", L"ュ", },               //[ly] : ｬ ｪ ｨ ｮ ｭ
  { L"ミャ", L"ミェ", L"ミィ", L"ミョ", L"ミュ", },     //[my] : ﾐｬ ﾐｪ ﾐｨ ﾐｮ ﾐｭ
  { L"ニャ", L"ニェ", L"ニィ", L"ニョ", L"ニュ", },     //[ny] : ﾆｬ ﾆｪ ﾆｨ ﾆｮ ﾆｭ
  { L"ピャ", L"ピェ", L"ピィ", L"ピョ", L"ピュ", },     //[py] : ﾋﾟｬ ﾋﾟｪ ﾋﾟｨ ﾋﾟｮ ﾋﾟｭ
  { L"クァ", L"クェ", L"クィ", L"クォ", L"クゥ", },     //[qw] : ｸｧ ｸｪ ｸｨ ｸｫ ｸｩ
  { L"クャ", L"クェ", L"クィ", L"クョ", L"クュ", },     //[qy] : ｸｬ ｸｪ ｸｨ ｸｮ ｸｭ
  { L"リャ", L"リェ", L"リィ", L"リョ", L"リュ", },     //[ry] : ﾘｬ ﾘｪ ﾘｨ ﾘｮ ﾘｭ
  { L"シャ", L"シェ", L"シ", L"ショ", L"シュ", },       //[sh] : ｼｬ ｼｪ ｼ ｼｮ ｼｭ
  { L"スァ", L"スェ", L"スィ", L"スォ", L"スゥ", },     //[sw] : ｽｧ ｽｪ ｽｨ ｽｫ ｽｩ
  { L"シャ", L"シェ", L"シィ", L"ショ", L"シュ", },     //[sy] : ｼｬ ｼｪ ｼｨ ｼｮ ｼｭ
  { L"テャ", L"テェ", L"ティ", L"テョ", L"テュ", },     //[th] : ﾃｬ ﾃｪ ﾃｨ ﾃｮ ﾃｭ
  { L"ツァ", L"ツェ", L"ツィ", L"ツォ", L"ツ", },       //[ts] : ﾂｧ ﾂｪ ﾂｨ ﾂｫ ﾂ
  { L"トァ", L"トェ", L"トィ", L"トォ", L"トゥ", },     //[tw] : ﾄｧ ﾄｪ ﾄｨ ﾄｫ ﾄｩ
  { L"チャ", L"チェ", L"チィ", L"チョ", L"チュ", },     //[ty] : ﾁｬ ﾁｪ ﾁｨ ﾁｮ ﾁｭ
  { L"ヴャ", L"ヴェ", L"ヴィ", L"ヴョ", L"ヴュ", },     //[vy] : ｳﾞｬ ｳﾞｪ ｳﾞｨ ｳﾞｮ ｳﾞｭ
  { L"ウァ", L"ウェ", L"ウィ", L"ウォ", L"ウ", },       //[wh] : ｳｧ ｳｪ ｳｨ ｳｫ ｳ
  { L"", L"", L"", L"", L"ッ", },                       //[xt] : NG NG NG NG ｯ
  { L"ャ", L"ェ", L"ィ", L"ョ", L"ュ", },               //[xy] : ｬ ｪ ｪ ｮ ｭ
  { L"ジャ", L"ジェ", L"ジィ", L"ジョ", L"ジュ", },     //[zy] : ｼﾞｬ ｼﾞｪ ｼﾞｨ ｼﾞｮ ｼﾞｭ
};

const wchar_t RomaHiragana[][5][3] PROGMEM =  {
  //  a      e      i      o      u
  { L"あ", L"え", L"い", L"お", L"う", },
  { L"ば", L"べ", L"び", L"ぼ", L"ぶ", },
  { L"か", L"せ", L"し", L"こ", L"く", },
  { L"だ", L"で", L"ぢ", L"ど", L"づ", },
  { L"ふぁ", L"ふぇ", L"ふぃ", L"ふぉ", L"ふ", },
  { L"が", L"げ", L"ぎ", L"ご", L"ぐ", },
  { L"は", L"へ", L"ひ", L"ほ", L"ふ", },
  { L"じゃ", L"じぇ", L"じ", L"じょ", L"じゅ", },
  { L"か", L"け", L"き", L"こ", L"く", },
  { L"ぁ", L"ぇ", L"ぃ", L"ぉ", L"ぅ", },
  { L"ま", L"め", L"み", L"も", L"む", },
  { L"な", L"ね", L"に", L"の", L"ぬ", },
  { L"ぱ", L"ぺ", L"ぴ", L"ぽ", L"ぷ", },
  { L"くぁ", L"くぇ", L"くぃ", L"くぉ", L"く", },
  { L"ら", L"れ", L"り", L"ろ", L"る", },
  { L"さ", L"せ", L"し", L"そ", L"す", },
  { L"た", L"て", L"ち", L"と", L"つ", },
  { L"", L"", L"", L"", L"", },
  { L"わ", L"うぇ", L"うぃ", L"を", L"う", },
  { L"ぁ", L"ぇ", L"ぃ", L"ぉ", L"ぅ", },
  { L"や", L"ぃぇ", L"い", L"よ", L"ゆ", },
  { L"ざ", L"ぜ", L"じ", L"ぞ", L"ず", },
  { L"びゃ", L"びぇ", L"びぃ", L"びょ", L"びゅ"},
  { L"ちゃ", L"ちぇ", L"ち", L"ちょ", L"ちゅ", },
  { L"ちゃ", L"ちぇ", L"ちぃ", L"ちょ", L"ちゅ", },
  { L"でゃ", L"でぇ", L"でぃ", L"でょ", L"でゅ", },
  { L"どぁ", L"どぇ", L"どぃ", L"どぉ", L"どぅ", },
  { L"ぢゃ", L"ぢぇ", L"ぢぃ", L"ぢょ", L"ぢゅ", },
  { L"ふぁ", L"ふぇ", L"ふぃ", L"ふぉ", L"ふぅ", },
  { L"ふゃ", L"ふぇ", L"ふぃ", L"ふょ", L"ふゅ", },
  { L"ぐぁ", L"ぐぇ", L"ぐぃ", L"ぐぉ", L"ぐぅ", },
  { L"ぎゃ", L"ぎぇ", L"ぎぃ", L"ぎょ", L"ぎゅ", },
  { L"ひゃ", L"ひぇ", L"ひぃ", L"ひょ", L"ひゅ", },
  { L"じゃ", L"じぇ", L"じぃ", L"じょ", L"じゅ", },
  { L"くぁ", L"", L"", L"", L"", },
  { L"きゃ", L"きぇ", L"きぃ", L"きょ", L"きゅ", },
  { L"", L"", L"", L"", L"っ", },
  { L"ゃ", L"ぇ", L"ぃ", L"ょ", L"ゅ", },
  { L"みゃ", L"みぇ", L"みぃ", L"みょ", L"みゅ", },
  { L"にゃ", L"にぇ", L"にぃ", L"にょ", L"にゅ", },
  { L"ぴゃ", L"ぴぇ", L"ぴぃ", L"ぴょ", L"ぴゅ", },
  { L"くぁ", L"くぇ", L"くぃ", L"くぉ", L"くぅ", },
  { L"くゃ", L"くぇ", L"くぃ", L"くょ", L"くゅ", },
  { L"りゃ", L"りぇ", L"りぃ", L"りょ", L"りゅ", },
  { L"しゃ", L"しぇ", L"し", L"しょ", L"しゅ", },
  { L"すぁ", L"すぇ", L"すぃ", L"すぉ", L"すぅ", },
  { L"しゃ", L"しぇ", L"しぃ", L"しょ", L"しゅ", },
  { L"てゃ", L"てぇ", L"てぃ", L"てょ", L"てゅ", },
  { L"つぁ", L"つぇ", L"つぃ", L"つぉ", L"つ", },
  { L"とぁ", L"とぇ", L"とぃ", L"とぉ", L"とぅ", },
  { L"ちゃ", L"ちぇ", L"ちぃ", L"ちょ", L"ちゅ", },
  { L"", L"", L"", L"", L"", },
  { L"うぁ", L"うぇ", L"うぃ", L"うぉ", L"う", },
  { L"", L"", L"", L"", L"っ", },
  { L"ゃ", L"ぇ", L"ぃ", L"ょ", L"ゅ", },
  { L"じゃ", L"じぇ", L"じぃ", L"じょ", L"じゅ", },
};
// clang-format on

// 例外([nn])
const wchar_t RomaKatakana_nn[4] __FLASH__ = L"ン";
const wchar_t RomaHiragana_nn[4] __FLASH__ = L"ん";

// 母音テーブル
const char BoonTable[] __FLASH__ = {
  'a', 'e', 'i', 'o', 'u',
};

// 子音テーブル
const char ShionTable[] __FLASH__ = {
  'b', 'c', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'n',
  'p', 'q', 'r', 's', 't', 'v', 'w', 'x', 'y', 'z',
};

// 2文字子音テーブル Xh系列
const char Shion_Xh_Table[][2] __FLASH__ = {
  { _romaji_c, _romaji_ch }, { _romaji_d, _romaji_dh },
  { _romaji_s, _romaji_sh }, { _romaji_t, _romaji_th },
  { _romaji_w, _romaji_wh },
};

// 2文字子音テーブル Xw系列
const char Shion_Xw_Table[][2] __FLASH__ = {
  { _romaji_d, _romaji_dw }, { _romaji_f, _romaji_fw },
  { _romaji_g, _romaji_gw }, { _romaji_k, _romaji_kw },
  { _romaji_q, _romaji_qw }, { _romaji_s, _romaji_sw },
  { _romaji_t, _romaji_tw },
};

// 2文字子音テーブル Xt系列
const char Shion_Xt_Table[][2] __FLASH__ = {
  { _romaji_x, _romaji_xt },
  { _romaji_l, _romaji_lt },
};

// 2文字子音テーブル Xy系列
const char Shion_Xy_Table[][2] __FLASH__ = {
  { _romaji_b, _romaji_by }, { _romaji_c, _romaji_xy },
  { _romaji_d, _romaji_dy }, { _romaji_f, _romaji_fy },
  { _romaji_g, _romaji_gy }, { _romaji_h, _romaji_hy },
  { _romaji_j, _romaji_jy }, { _romaji_k, _romaji_ky },
  { _romaji_l, _romaji_ly }, { _romaji_m, _romaji_my },
  { _romaji_n, _romaji_ny }, { _romaji_p, _romaji_py },
  { _romaji_q, _romaji_qy }, { _romaji_r, _romaji_ry },
  { _romaji_s, _romaji_sy }, { _romaji_t, _romaji_ty },
  { _romaji_v, _romaji_vy }, { _romaji_x, _romaji_xy },
  { _romaji_z, _romaji_zy },
};

int16_t romaji_sts = _romaji_top;  // State transition code
uint8_t flgTsu = false;            // 小さいﾂ付加フラグ
wchar_t kataStr[6];                // 確定カタカナ文字列

// 文字コードから母音コード(0～4)に変換する
inline int16_t charToBoonCode(uint8_t c) {
  for (uint8_t i = 0; i < sizeof(BoonTable); i++)
    if (c == pgm_read_byte(&BoonTable[i]))
      return i;
  return -1;
}

// 文字コードから子音コードに変換する
inline int16_t charToShionCode(uint8_t c) {
  for (uint8_t i = 0; i < sizeof(ShionTable); i++)
    if (c == pgm_read_byte(&ShionTable[i]))
      return i + 1;
  return -1;
}

// Romaji Katakana conversion
// Transition from the previous state transition to the next state
wchar_t *pRomaji2Kana(uint8_t c) {
  int16_t code;
  wchar_t *ptr;

  // 小文字変換
  if (c >= 'A' && c <= 'Z')
    c = c - 'A' + 'a';

  // 文字範囲チェック
  // (後で長音・濁音・半濁音・句点・読点の許可する対応の追加)
  if (c < 'a' || c > 'z')
    goto STS_ERROR;

  code = charToBoonCode(c);  // 母音チェック
  if (code >= 0) {
    // 母音の場合,文字列を確定する
    if (romaji_sts >= _romaji_top && romaji_sts <= _romaji_zy) {
      ptr = (wchar_t *)(flgKana == 1 ? RomaKatakana : RomaHiragana)[romaji_sts][code];
      goto STS_DONE;  // 変換完了
    } else
      goto STS_ERROR;  // 変換エラー
  } else {
    // 母音でない場合、子音コードを取得
    code = charToShionCode(c);
    if (code < 0)
      goto STS_ERROR;  // 子音でない(エラー)
    if (romaji_sts == _romaji_top) {
      // 初期状態で子音コードなら次の状態に遷移
      romaji_sts = code;
      goto STS_NEXT;
    } else if (romaji_sts >= _romaji_b && romaji_sts <= _romaji_z) {
      // 1文字子音受理済みからの子音入力の対応
      if (romaji_sts == code) {
        // 同じ子音が連続
        if (!flgTsu) {
          if (code == _romaji_n) {
            // nn('ﾝ')の場合
            ptr = (wchar_t *)(flgKana == 1 ? RomaKatakana_nn : RomaHiragana_nn);
            goto STS_DONE;  // 変換完了
          } else {
            flgTsu = true;  // 小さい'ﾂ'の先頭付加フラグの設定
            goto STS_NEXT;
          }
        } else
          // 既に小さい'ﾂ'の先頭付加フラグがセットされている場合はエラー
          goto STS_ERROR;
      } else {
        // 2文字子音への遷移チェック
        switch (code) {
        case _romaji_h:
          for (uint16_t i=0; i < (sizeof(Shion_Xh_Table)/sizeof(Shion_Xh_Table[0])); i++)
            if ( pgm_read_byte(&Shion_Xh_Table[i][0]) == romaji_sts) {
              romaji_sts =  pgm_read_byte(&Shion_Xh_Table[i][1]);
              goto STS_NEXT;
            }
          goto STS_ERROR;
        case _romaji_w:
          for (uint16_t i=0; i < (sizeof(Shion_Xw_Table)/sizeof(Shion_Xw_Table[0])); i++)
            if ( pgm_read_byte(&Shion_Xw_Table[i][0]) == romaji_sts) {
              romaji_sts =  pgm_read_byte(&Shion_Xw_Table[i][1]);
              goto STS_NEXT;
            }
          goto STS_ERROR;
        case _romaji_t:
          for (uint16_t i=0; i < (sizeof(Shion_Xt_Table)/sizeof(Shion_Xt_Table[0])); i++)
            if ( pgm_read_byte(&Shion_Xt_Table[i][0]) == romaji_sts) {
              romaji_sts = pgm_read_byte(&Shion_Xt_Table[i][1]);
              goto STS_NEXT;
            }
          goto STS_ERROR;
        case _romaji_y:
          for (uint16_t i=0; i < (sizeof(Shion_Xy_Table)/sizeof(Shion_Xy_Table[0])); i++)
            if ( pgm_read_byte(&Shion_Xy_Table[i][0]) == romaji_sts) {
              romaji_sts = pgm_read_byte(&Shion_Xy_Table[i][1]);
              goto STS_NEXT;
            }
          goto STS_ERROR;
        default:
          goto STS_ERROR;
        }
      }
    }
  }

STS_NEXT:  // 次の状態へ
  return NULL;

STS_ERROR: // [状態遷移エラー]
  romaji_sts = _romaji_top;  // 状態の初期化
  flgTsu = false;            // 小さい'ﾂ'の先頭付加フラグクリア
  return NULL;

STS_DONE:  // [Romaji Katakana conversion transition completed]
  if (flgTsu) {
    kataStr[0] = flgKana == 1 ? L'ッ' : L'っ';  // 'ｯ' setting
    wcscpy(kataStr + 1, ptr);
    ptr = kataStr;
  }
  romaji_sts = _romaji_top;  // State initialization
  flgTsu = false;            // Clear the flag added to the beginning of a small'tsu'
  return ptr;
}

struct cbm_map {
  unsigned int key;
  utf8_int32_t map;
};

static const cbm_map cbm_alt_keymap[] = {
  { '^', 0x03c0 },
  { '*', 0x2500 },
  { '-', 0x2502 },
  { '+', 0x253c },
  { 'a', 0x2660 },
  { 'b', 0x2502 },
  { 'c', 0x2500 },
  { 'd', 0xe0c4 },
  { 'e', 0xe0c5 },
  { 'f', 0xe0c6 },
  { 'g', 0xe0c7 },
  { 'h', 0xe0c8 },
  { 'i', 0x256e },
  { 'j', 0x2570 },
  { 'k', 0x256f },
  { 'l', 0xe0cc },
  { 'm', 0x2572 },
  { 'n', 0x2571 },
  { 'o', 0xe0cf },
  { 'p', 0xe0d0 },
  { 'q', 0x25cf },
  { 'r', 0xe0d2 },
  { 's', 0x2665 },
  { 't', 0xe0d4 },
  { 'u', 0x256d },
  { 'v', 0x2573 },
  { 'w', 0x25cb },
  { 'x', 0x2663 },
  { 'y', 0xe0d9 },
  { 'z', 0x2666 },
  // XXX: shift-pound == 0x25e4
};

static const cbm_map cbm_gui_keymap[] = {
  { '@', 0x2581 },
  { '+', 0x2592 },
  { '*', 0x25e5 },
  { '-', 0xe0dc },
  { 'a', 0x250c },
  { 'b', 0x259a },
  { 'c', 0x259d },
  { 'd', 0x2597 },
  { 'e', 0x2534 },
  { 'f', 0x2596 },
  { 'g', 0x258e },
  { 'h', 0x258e },
  { 'i', 0x2584 },
  { 'j', 0x258d },
  { 'k', 0x258c },
  { 'l', 0x258b },
  { 'm', 0x258a },
  { 'n', 0x258a },
  { 'o', 0x2583 },
  { 'p', 0x2582 },
  { 'q', 0x251c },
  { 'r', 0x252c },
  { 's', 0x2510 },
  { 't', 0x2594 },
  { 'u', 0x2585 },
  { 'v', 0x2598 },
  { 'w', 0x2524 },
  { 'x', 0x2518 },
  { 'y', 0x2586 },
  { 'z', 0x2514 },
  // XXX: CBM+pound == 0xe0a8
};

// PS/2 keyboard setup
void setupPS2(uint8_t kb_type = 0) {
  // Initialize keyboard with LED control enabled
  if (kb.begin(IRQpin, DataPin, true, kb_type)) {
    //Serial.println("PS/2 Keyboard initialize error.");
  }

  rb_init(&kbuf, sizeof(kdata) / sizeof(kdata[0]), kdata);
  //kb.setPriority(0);
}

void endPS2() {
  kb.end();
}

// PS/2キーボード入力処理
uint16_t cnv2tty(keyEvent k) {
  uint16_t rc = 0;
  if (!k.code || k.BREAK)
    return 0;

  if (k.KEY && k.code >= PS2KEY_F1 && k.code <= PS2KEY_F12) {
    return SC_KEY_F(k.code - PS2KEY_F1 + 1);
  }

  // CTRLとの併用
  if (k.CTRL) {
    //Serial.println(k.code,DEC);
    if (!k.KEY) {
      if (k.code == '\r')
        rc =  '\r';
      else if (k.code >= 'a' && k.code <= 'z') {
        rc = k.code - 'a' + 1;
      } else if (k.code >= 'A' && k.code <= 'Z') {
        rc = k.code - 'A' + 1;
      } else {
        switch(k.code) {
          case PS2KEY_AT:         rc = 0;    break;
          case PS2KEY_L_brackets: rc = 0x1b; break;
          case PS2KEY_Pipe2:      rc = 0x1c; break;
          case PS2KEY_R_brackets: rc = 0x1d; break;
          case PS2KEY_Hat:        rc = 0x1e; break;
          case PS2KEY_Ro:         rc = 0x1f; break;
          case ' ':               flgKana = (flgKana + 1) % 3; rc = 0; break;
        }
      }
      return rc;
    }
  } else if (k.ALT && k.SHIFT && !k.KEY) {
    for (auto m : cbm_gui_keymap) {
      if (m.key == tolower(k.code))
        return m.map;
    }
  } else if (k.ALT && !k.KEY) {
    for (auto m : cbm_alt_keymap) {
      if (m.key == k.code)
        return m.map;
    }
  }

  // 通常のASCIIコード
  if (!k.KEY) {
    rc = k.code;
    return rc;
  }

  // 特殊キーの場合
  switch(k.code) {
    case PS2KEY_Insert:     rc = SC_KEY_IC;       break;
    case PS2KEY_Home:       rc = SC_KEY_HOME;     break;
    case PS2KEY_PageUp:     rc = SC_KEY_PPAGE;    break;
    case PS2KEY_PageDown:   rc = SC_KEY_NPAGE;    break;
    case PS2KEY_End:        rc = SC_KEY_END;      break;
    case PS2KEY_L_Arrow:    rc = SC_KEY_LEFT;     break;
    case PS2KEY_Up_Arrow:   rc = k.SHIFT ? SC_KEY_SHIFT_UP : SC_KEY_UP;	break;
    case PS2KEY_R_Arrow:    rc = SC_KEY_RIGHT;    break;
    case PS2KEY_Down_Arrow: rc = k.SHIFT ? SC_KEY_SHIFT_DOWN : SC_KEY_DOWN;	break;
    case PS2KEY_ESC:        rc = SC_KEY_ESCAPE;   break;
    case PS2KEY_Tab:        rc = SC_KEY_TAB;      break;
    case PS2KEY_Space:
      if (k.CTRL)
        flgKana = (flgKana + 1) % 3;
      else
        rc = ' ';
      break;
    case PS2KEY_Backspace:  rc = SC_KEY_BACKSPACE;break;
    case PS2KEY_Delete:     rc = SC_KEY_DC;       break;
    case PS2KEY_Enter:	    rc = SC_KEY_CR;       break;
    case PS2KEY_Romaji:     flgKana = (flgKana + 1) % 3; break;
    case PS2KEY_PrintScreen: rc = SC_KEY_PRINT; break;
  }
  return rc;
}

keyEvent ps22tty_last_key;

// キー入力文字の取得
uint16_t ICACHE_RAM_ATTR ps2read() {
  wchar_t *ptr;
  uint16_t c;
  uint8_t len;

  if (pb.available() && (ps22tty_last_key = kb.read(), ps22tty_last_key.code))
    c = cnv2tty(ps22tty_last_key);
  else
    c = 0;

  if (c == 0) {
    if (!rb_is_empty(&kbuf))
      return rb_remove(&kbuf);
    else
      return 0;
  }

  if (flgKana != 0) {
    ptr = pRomaji2Kana(c);
    if (ptr) {
      len = wcslen(ptr);
      for (int16_t i = 0; i < len; i++) {
        rb_insert(&kbuf, ptr[i]);
      }
    } else {
      if (romaji_sts == _romaji_top) {
        switch(c) {
          case '[': c = 0xa2;break;
          case ']': c = 0xa3;break;
          case ',': c = 0xa4;break;
          case '.': c = 0xa1;break;
          case '@': c = 0xde;break;
          case '{': c = 0xdf;break;
          case '-': c = 0xb0;break;
        }
        return c;

      } else
        return 0;
    }

    if (!rb_is_empty(&kbuf)) {
      return rb_remove(&kbuf);
    } else {
      return 0;
    }
  }
  return c;
}

uint16_t ICACHE_RAM_ATTR ps2peek() {
  uint16_t c;
  if (ps2kbhit()) {
    c = ps2read();
    if (c) {
      rb_insert(&kbuf, c);
    }
    if (!rb_is_empty(&kbuf))
      return kbuf.buf[kbuf.head];
  }
  return 0;
}

#endif
