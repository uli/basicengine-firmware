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

#if defined(ESP8266) && !defined(__FLASH__)
#define __FLASH__ ICACHE_RODATA_ATTR
#endif

#ifndef ESP8266_NOWIFI
// workaround until there is a basic_engine board in mainline Arduino
static const uint8_t PS2CLK = 4;
static const uint8_t PS2DAT = 5;
#endif

#include "ps22tty.h"

const int IRQpin =  PS2CLK;  // CLK(D+)
const int DataPin = PS2DAT;  // Data(D-) 

TKeyboard kb;               // PS/2キーボードドライバ
uint8_t  flgKana = false;    // カナ入力モード
struct   ring_buffer kbuf;   // キーバッファ
uint16_t  kdata[16];  

#define RK_ENTRY_NUM (sizeof(RomaKama)/sizeof(RomaKama[0]))

//子音状態遷移コード
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
 const char RomaKama[][5][4] ICACHE_RODATA_ATTR =  {
  //  a       e       i       o        u
  { "\xb1", "\xb4", "\xb2", "\xb5", "\xb3", },                                          //[]  : ｱ ｴ ｲ ｵ ｳ
  { "\xca\xde", "\xcd\xde", "\xcb\xde", "\xce\xde", "\xcc\xde", },                      //[b] : ﾊﾞ ﾍﾞ ﾋﾞ ﾎﾞ ﾌﾞ
  { "\xb6", "\xbe", "\xbc", "\xba", "\xb8", },                                          //[c] : ｶ ｾ ｼ ｺ ｸ
  { "\xc0\xde", "\xc3\xde", "\xc1\xde", "\xc4\xde", "\xc2\xde", },                      //[d] : ﾀﾞ ﾃﾞ ﾁﾞ ﾄﾞ ﾂﾞ
  { "\xcc\xa7", "\xcc\xaa", "\xcc\xa8", "\xcc\xab", "\xcc", },                          //[f] : ﾌｧ ﾌｪ ﾌｨ ﾌｫ ﾌ
  { "\xb6\xde", "\xb9\xde", "\xb7\xde", "\xba\xde", "\xb8\xde", },                      //[g] : ｶﾞ ｹﾞ ｷﾞ ｺﾞ ｸﾞ
  { "\xca", "\xcd", "\xcb", "\xce", "\xcc", },                                          //[h] : ﾊ ﾍ ﾋ ﾎ ﾌ
  { "\xbc\xde\xac", "\xbc\xde\xaa", "\xbc\xde", "\xbc\xde\xae", "\xbc\xde\xad", },      //[j] : ｼﾞｬ ｼﾞｪ ｼﾞ ｼﾞｮ ｼﾞｭ
  { "\xb6", "\xb9", "\xb7", "\xba", "\xb8", },                                          //[k] : ｶ ｹ ｷ ｺ ｸ
  { "\xa7", "\xaa", "\xa8", "\xab", "\xa9", },                                          //[l] : ｧ ｪ ｨ ｫ ｩ
  { "\xcf", "\xd2", "\xd0", "\xd3", "\xd1", },                                          //[m] : ﾏ ﾒ ﾐ ﾓ ﾑ
  { "\xc5", "\xc8", "\xc6", "\xc9", "\xc7", },                                          //[n] : ﾅ ﾈ ﾆ ﾉ ﾇ
  { "\xca\xdf", "\xcd\xdf", "\xcb\xdf", "\xce\xdf", "\xcc\xdf", },                      //[p] : ﾊﾟ ﾍﾟ ﾋﾟ ﾎﾟ ﾌﾟ
  { "\xb8\xa7", "\xb8\xaa", "\xb8\xa8", "\xb8\xab", "\xb8", },                          //[q] : ｸｧ ｸｪ ｸｨ ｸｫ ｸ
  { "\xd7", "\xda", "\xd8", "\xdb", "\xd9", },                                          //[r] : ﾗ ﾚ ﾘ ﾛ ﾙ
  { "\xbb", "\xbe", "\xbc", "\xbf", "\xbd", },                                          //[s] : ｻ ｾ ｼ ｿ ｽ
  { "\xc0", "\xc3", "\xc1", "\xc4", "\xc2", },                                          //[t] : ﾀ ﾃ ﾁ ﾄ ﾂ
  { "\xb3\xde\xa7", "\xb3\xde\xaa", "\xb3\xde\xa8", "\xb3\xde\xab", "\xb3\xde", },      //[v] : ｳﾞｧ ｳﾞｪ ｳﾞｨ ｳﾞｫ ｳﾞ
  { "\xdc", "\xb3\xaa", "\xb3\xa8", "\xa6", "\xb3", },                                  //[w] : ﾜ ｳｪ ｳｨ ｦ ｳ
  { "\xa7", "\xaa", "\xa8", "\xab", "\xa9", },                                          //[x] : ｧ ｪ ｨ ｫ ｩ
  { "\xd4", "\xa8", "\xb2", "\xd6", "\xd5", },                                          //[y] : ﾔ ｨｪ ｲ ﾖ ﾕ
  { "\xbb\xde", "\xbe\xde", "\xbc\xde", "\xbf\xde", "\xbd\xde", },                      //[z] : ｻﾞ ｾﾞ ｼﾞ ｿﾞ ｽﾞ
  { "\xcb\xde\xac", "\xcb\xde\xaa", "\xcb\xde\xa8", "\xcb\xde\xae", "\xcb\xde\xad",},   //[by] : ﾋﾞｬ ﾋﾞｪ ﾋﾞｨ ﾋﾞｮ ﾋﾞｭ
  { "\xc1\xac", "\xc1\xaa", "\xc1", "\xc1\xae", "\xc1\xad", },                          //[ch] : ﾁｬ ﾁｪ ﾁ ﾁｮ ﾁｭ
  { "\xc1\xac", "\xc1\xaa", "\xc1\xa8", "\xc1\xae", "\xc1\xad", },                      //[cy] : ﾁｬ ﾁｪ ﾁｨ ﾁｮ ﾁｭ
  { "\xc3\xde\xac", "\xc3\xde\xaa", "\xc3\xde\xa8", "\xc3\xde\xae", "\xc3\xde\xad", },  //[dh] : ﾃﾞｬ ﾃﾞｪ ﾃﾞｨ ﾃﾞｮ ﾃﾞｭ
  { "\xc4\xde\xa7", "\xc4\xde\xaa", "\xc4\xde\xa8", "\xc4\xde\xab", "\xc4\xde\xa9", },  //[dw] : ﾄﾞｧ ﾄﾞｪ ﾄﾞｨ ﾄﾞｫ ﾄﾞｩ
  { "\xc1\xde\xac", "\xc1\xde\xaa", "\xc1\xde\xa8", "\xc1\xde\xae", "\xc1\xde\xad", },  //[dy] : ﾁﾞｬ ﾁﾞｪ ﾁﾞｨ ﾁﾞｮ ﾁﾞｭ
  { "\xcc\xa7", "\xcc\xaa", "\xcc\xa8", "\xcc\xab", "\xcc\xa9", },                      //[fw] : ﾌｧ ﾌｪ ﾌｨ ﾌｫ ﾌｩ
  { "\xcc\xac", "\xcc\xaa", "\xcc\xa8", "\xcc\xae", "\xcc\xad", },                      //[fy] : ﾌｬ ﾌｪ ﾌｨ ﾌｮ ﾌｭ
  { "\xb8\xde\xa7", "\xb8\xde\xaa", "\xb8\xde\xa8", "\xb8\xde\xab", "\xb8\xde\xa9", },  //[gw] : ｸﾞｧ ｸﾞｪ ｸﾞｨ ｸﾞｫ ｸﾞｩ
  { "\xb7\xde\xac", "\xb7\xde\xaa", "\xb7\xde\xa8", "\xb7\xde\xae", "\xb7\xde\xad", },  //[gy] : ｷﾞｬ ｷﾞｪ ｷﾞｨ ｷﾞｮ ｷﾞｭ
  { "\xcb\xac", "\xcb\xaa", "\xcb\xa8", "\xcb\xae", "\xcb\xad", },                      //[hy] : ﾋｬ ﾋｪ ﾋｨ ﾋｮ ﾋｭ
  { "\xbc\xde\xac", "\xbc\xde\xaa", "\xbc\xde\xa8", "\xbc\xde\xae", "\xbc\xde\xad", },  //[jy] : ｼﾞｬ ｼﾞｪ ｼﾞｨ ｼﾞｮ ｼﾞｭ
  { "\xb8\xa7", "\x00", "\x00", "\x00", "\x00", },                                      //[kw] : ｸｧ NG NG NG NG
  { "\xb7\xac", "\xb7\xaa", "\xb7\xa8", "\xb7\xae", "\xb7\xad", },                      //[ky] : ｷｬ ｷｪ ｷｨ ｷｮ ｷｭ
  { "\x00", "\x00", "\x00", "\x00", "\xaf", },                                          //[lt] : NG NG NG NG ｯ
  { "\xac", "\xaa", "\xa8", "\xae", "\xad", },                                          //[ly] : ｬ ｪ ｨ ｮ ｭ
  { "\xd0\xac", "\xd0\xaa", "\xd0\xa8", "\xd0\xae", "\xd0\xad", },                      //[my] : ﾐｬ ﾐｪ ﾐｨ ﾐｮ ﾐｭ
  { "\xc6\xac", "\xc6\xaa", "\xc6\xa8", "\xc6\xae", "\xc6\xad", },                      //[ny] : ﾆｬ ﾆｪ ﾆｨ ﾆｮ ﾆｭ
  { "\xcb\xdf\xac", "\xcb\xdf\xaa", "\xcb\xdf\xa8", "\xcb\xdf\xae", "\xcb\xdf\xad", },  //[py] : ﾋﾟｬ ﾋﾟｪ ﾋﾟｨ ﾋﾟｮ ﾋﾟｭ
  { "\xb8\xa7", "\xb8\xaa", "\xb8\xa8", "\xb8\xab", "\xb8\xa9", },                      //[qw] : ｸｧ ｸｪ ｸｨ ｸｫ ｸｩ
  { "\xb8\xac", "\xb8\xaa", "\xb8\xa8", "\xb8\xae", "\xb8\xad", },                      //[qy] : ｸｬ ｸｪ ｸｨ ｸｮ ｸｭ
  { "\xd8\xac", "\xd8\xaa", "\xd8\xa8", "\xd8\xae", "\xd8\xad", },                      //[ry] : ﾘｬ ﾘｪ ﾘｨ ﾘｮ ﾘｭ
  { "\xbc\xac", "\xbc\xaa", "\xbc", "\xbc\xae", "\xbc\xad", },                          //[sh] : ｼｬ ｼｪ ｼ ｼｮ ｼｭ
  { "\xbd\xa7", "\xbd\xaa", "\xbd\xa8", "\xbd\xab", "\xbd\xa9", },                      //[sw] : ｽｧ ｽｪ ｽｨ ｽｫ ｽｩ
  { "\xbc\xac", "\xbc\xaa", "\xbc\xa8", "\xbc\xae", "\xbc\xad", },                      //[sy] : ｼｬ ｼｪ ｼｨ ｼｮ ｼｭ
  { "\xc3\xac", "\xc3\xaa", "\xc3\xa8", "\xc3\xae", "\xc3\xad", },                      //[th] : ﾃｬ ﾃｪ ﾃｨ ﾃｮ ﾃｭ
  { "\xc2\xa7", "\xc2\xaa", "\xc2\xa8", "\xc2\xab", "\xc2", },                          //[ts] : ﾂｧ ﾂｪ ﾂｨ ﾂｫ ﾂ
  { "\xc4\xa7", "\xc4\xaa", "\xc4\xa8", "\xc4\xab", "\xc4\xa9", },                      //[tw] : ﾄｧ ﾄｪ ﾄｨ ﾄｫ ﾄｩ
  { "\xc1\xac", "\xc1\xaa", "\xc1\xa8", "\xc1\xae", "\xc1\xad", },                      //[ty] : ﾁｬ ﾁｪ ﾁｨ ﾁｮ ﾁｭ
  { "\xb3\xde\xac", "\xb3\xde\xaa", "\xb3\xde\xa8", "\xb3\xde\xae", "\xb3\xde\xad", },  //[vy] : ｳﾞｬ ｳﾞｪ ｳﾞｨ ｳﾞｮ ｳﾞｭ
  { "\xb3\xa7", "\xb3\xaa", "\xb3\xa8", "\xb3\xab", "\xb3", },                          //[wh] : ｳｧ ｳｪ ｳｨ ｳｫ ｳ
  { "\x00", "\x00", "\x00", "\x00", "\xaf", },                                          //[xt] : NG NG NG NG ｯ
  { "\xac", "\xaa", "\xaa", "\xae", "\xad", },                                          //[xy] : ｬ ｪ ｪ ｮ ｭ
  { "\xbc\xde\xac", "\xbc\xde\xaa", "\xbc\xde\xa8", "\xbc\xde\xae", "\xbc\xde\xad", },  //[zy] : ｼﾞｬ ｼﾞｪ ｼﾞｨ ｼﾞｮ ｼﾞｭ
};

// 例外([nn])
const char RomaKama_nn[4] __FLASH__ ="\xdd"; // 'ﾝ'

// 母音テーブル
const char BoonTable[] __FLASH__ = { 
  'a','e','i','o','u',
};

// 子音テーブル
const char ShionTable[] __FLASH__ = {
  'b' ,'c' ,'d' ,'f' ,'g' ,'h' ,'j' ,'k' ,'l' ,'m' ,'n' ,'p' ,'q' ,'r' ,'s' ,'t' ,'v' ,'w' ,'x' ,'y' ,'z',
};

// 2文字子音テーブル Xh系列
const char Shion_Xh_Table[][2] __FLASH__ = {
  { _romaji_c, _romaji_ch },{ _romaji_d, _romaji_dh },{ _romaji_s, _romaji_sh }, { _romaji_t, _romaji_th },
  { _romaji_w, _romaji_wh },  
};

// 2文字子音テーブル Xw系列
const char Shion_Xw_Table[][2] __FLASH__ = {
  { _romaji_d, _romaji_dw },{ _romaji_f, _romaji_fw },{ _romaji_g, _romaji_gw },{ _romaji_k, _romaji_kw }, 
  { _romaji_q, _romaji_qw },{ _romaji_s, _romaji_sw },{ _romaji_t, _romaji_tw }, 
};

// 2文字子音テーブル Xt系列
const char Shion_Xt_Table[][2] __FLASH__ = {
  {  _romaji_x, _romaji_xt } ,  {  _romaji_l, _romaji_lt } ,
};

// 2文字子音テーブル Xy系列
const char Shion_Xy_Table[][2] __FLASH__ = {
  { _romaji_b, _romaji_by },{ _romaji_c, _romaji_xy },{ _romaji_d, _romaji_dy },{ _romaji_f, _romaji_fy },
  { _romaji_g, _romaji_gy },{ _romaji_h, _romaji_hy },{ _romaji_j, _romaji_jy },{ _romaji_k, _romaji_ky },
  { _romaji_l, _romaji_ly },{ _romaji_m, _romaji_my },{ _romaji_n, _romaji_ny },{ _romaji_p, _romaji_py }, 
  { _romaji_q, _romaji_qy },{ _romaji_r, _romaji_ry },{ _romaji_s, _romaji_sy },{ _romaji_t, _romaji_ty },
  { _romaji_v, _romaji_vy },{ _romaji_x, _romaji_xy },{ _romaji_z, _romaji_zy }, 
};

int16_t romaji_sts = _romaji_top;  // 状態遷移コード
uint8_t flgTsu = false;            // 小さいﾂ付加フラグ
char kataStr[6];                   // 確定カタカナ文字列

// 文字コードから母音コード(0～4)に変換する
inline int16_t charToBoonCode(uint8_t c) {
  for (uint8_t i=0; i < sizeof(BoonTable); i++)
    if (c == pgm_read_byte(&BoonTable[i]))
       return i;
  return -1;
}

// 文字コードから子音コードに変換する
inline int16_t charToShionCode(uint8_t c) {
  for (uint8_t i=0; i < sizeof(ShionTable); i++)
    if (c == pgm_read_byte(&ShionTable[i]))
       return i+1;
  return -1;
}

// ローマ字カタカタ変換
// 直前の状態遷移から次の状態に遷移する
char* pRomaji2Kana(uint8_t c) {
  int16_t code;
  char*   ptr;

  // 小文字変換
  if (c >= 'A' && c <= 'Z')
    c = c - 'A' + 'a';

  // 文字範囲チェック
  // (後で長音・濁音・半濁音・句点・読点の許可する対応の追加)
  if (c < 'a' || c > 'z')
    goto STS_ERROR;
  
  code = charToBoonCode(c); // 母音チェック
  if (code >= 0) {
    // 母音の場合,文字列を確定する
    if (romaji_sts >= _romaji_top && romaji_sts <= _romaji_zy) {
      ptr = (char*)RomaKama[romaji_sts][code];
      goto STS_DONE;    // 変換完了
    } else
      goto STS_ERROR;  // 変換エラー
  } else {
    // 母音でない場合、子音コードを取得
    code = charToShionCode(c);
    if (code < 0) 
       goto STS_ERROR; // 子音でない(エラー)         
    if (romaji_sts == _romaji_top) {
      // 初期状態で子音コードなら次の状態に遷移
      romaji_sts = code;
      goto STS_NEXT;
    } else if (romaji_sts >= _romaji_b && romaji_sts <= _romaji_z) {
      // 1文字子音受理済みからの子音入力の対応
      if ( romaji_sts == code) {
        // 同じ子音が連続
        if (!flgTsu) {
           if (code == _romaji_n) {
             // nn('ﾝ')の場合
             ptr = (char*)RomaKama_nn;
             goto STS_DONE;    // 変換完了             
           } else {
             flgTsu = true;  // 小さい'ﾂ'の先頭付加フラグの設定
             goto STS_NEXT;
           }
        } else
          // 既に小さい'ﾂ'の先頭付加フラグがセットされている場合はエラー
          goto STS_ERROR;
      } else {
        // 2文字子音への遷移チェック
        switch(code) {
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
  
STS_DONE:  // [ローマ字カタカナ変換 遷移完了]
  if (flgTsu) {
    kataStr[0] = 0xaf; // 'ｯ' の設定
    strcpy(kataStr+1, ptr);
    ptr = kataStr;
  }
  romaji_sts = _romaji_top;  // 状態の初期化
  flgTsu = false;            // 小さい'ﾂ'の先頭付加フラグクリア
  return ptr;

}

// PS/2 keyboard setup
void setupPS2(uint8_t kb_type = 0) {
  // Initialize keyboard with LED control enabled
  if ( kb.begin(IRQpin, DataPin, true, kb_type) ) {
    //Serial.println("PS/2 Keyboard initialize error.");
  } 
  
  rb_init(&kbuf, sizeof(kdata)/sizeof(kdata[0]), kdata);
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

  // CTRLとの併用
  if (k.CTRL) {
    //Serial.println(k.code,DEC);
    if(!k.KEY) {
      if (k.code >= 'a' && k.code <= 'z') {
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
        }
      }
      if (rc == 11) {
        if (flgKana) {
          flgKana = false;
        } else {
          flgKana = true;
        }
        return 0;
      }
      return rc;
    }
  }
  
  // 通常のASCIIコード
  if(!k.KEY) {
    rc = k.code;
    return rc;
  }
  
  // 特殊キーの場合
  switch(k.code) {
    case PS2KEY_Insert:     rc = KEY_IC;       break;
    case PS2KEY_Home:       rc = KEY_HOME;     break;
    case PS2KEY_PageUp:     rc = KEY_PPAGE;    break;
    case PS2KEY_PageDown:   rc = KEY_NPAGE;    break;
    case PS2KEY_End:        rc = KEY_END;      break;
    case PS2KEY_L_Arrow:    rc = KEY_LEFT;     break;
    case PS2KEY_Up_Arrow:   rc = k.SHIFT ? KEY_SHIFT_UP : KEY_UP;	break;
    case PS2KEY_R_Arrow:    rc = KEY_RIGHT;    break;
    case PS2KEY_Down_Arrow: rc = k.SHIFT ? KEY_SHIFT_DOWN : KEY_DOWN;	break;
    case PS2KEY_ESC:        rc = KEY_ESCAPE;   break;
    case PS2KEY_Tab:        rc = KEY_TAB;      break;
    case PS2KEY_Space:      rc = 32;           break;
    case PS2KEY_Backspace:  rc = KEY_BACKSPACE;break;
    case PS2KEY_Delete:     rc = KEY_DC;       break;
    case PS2KEY_Enter:	    rc = KEY_CR;       break;
    case 112:            rc = SC_KEY_CTRL_L;break;
    case PS2KEY_F2:         rc = SC_KEY_CTRL_D;break;
    case PS2KEY_F3:         rc = SC_KEY_CTRL_N;break;
    case PS2KEY_F5:         rc = SC_KEY_CTRL_R;break;
    case PS2KEY_Romaji: 
      if (flgKana) {
        flgKana = false;
      } else {
        flgKana = true;
      }
      break;
  }
  return rc;
}

keyEvent ps22tty_last_key;

// キー入力文字の取得
uint16_t ICACHE_RAM_ATTR ps2read() {
  char* ptr;
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
  
  if (flgKana) {
    ptr = pRomaji2Kana(c);
    if (ptr) {
      len = strlen_P(ptr);
      for (int16_t i = 0 ; i < len; i++) {
        rb_insert(&kbuf, pgm_read_byte(&ptr[i]));
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

uint16_t ICACHE_RAM_ATTR ps2peek()
{
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
