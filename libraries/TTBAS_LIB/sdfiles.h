//
// sdfiles.h
// SDカードファイル操作クラス
// 作成日 2017/06/04 by たま吉さん
// 参考情報
// - Topic: SD Timestamp read? (Read 1 time) http://forum.arduino.cc/index.php?topic=114181.0
//
// 修正履歴
//  2017/07/27, flist()で列数を指定可能
//

#ifndef __sdfiles_h__
#define __sdfiles_h__

#include <Arduino.h>
#include <SdFat.h>

#define SD_CS       (4)   // SDカードモジュールCS
#define SD_PATH_LEN 64      // ディレクトリパス長
#define SD_TEXT_LEN 255     // テキスト１行最大長さ

#define SD_ERR_INIT       1    // SDカード初期化失敗
#define SD_ERR_OPEN_FILE  2    // ファイルオープン失敗
#define SD_ERR_READ_FILE  3    // ファイル読込失敗
#define SD_ERR_NOT_FILE   4    // ファイルでない(ディレクトリ)
#define SD_ERR_WRITE_FILE 5    //  ファイル書込み失敗
class sdfiles {
 private:

  File tfile;
  uint8_t flgtmpOlen;
  uint8_t cs;   
  bool SD_BEGIN(void);
  bool SD_END(void);

 public:
  uint8_t init(uint8_t cs=SD_CS);                       // 初期設定
  uint8_t load(char* fname, uint8_t* ptr, uint16_t sz); // ファイルのロード
  uint8_t save(char* fname, uint8_t* ptr, uint16_t sz); // ファイルのセーブ
  uint8_t flist(char* _dir, char* wildcard=NULL, uint8_t clmnum=2); // ファイルリスト出力
  uint8_t mkdir(char* fname);                           // ディレクトリの作成
  uint8_t rmdir(char* fname);                           // ディレクトリの削除
  //uint8_t rename(char* old_fname,char* new_fname);    // ファイル名の変更
  uint8_t remove(char* fname);                          // ファイルの削除
  uint8_t fcopy(char* old_fname,char* new_fname);       // ファイルのコピー

  uint8_t tmpOpen(char* tfname, uint8_t mode);          // 一時ファイルオープン
  uint8_t tmpClose();                                   // 一時ファイルクローズ
  uint8_t puts(char*s);                                 // 文字列出力
  int16_t read();                                       // 1バイト読込
  uint8_t putch(char c);                                // 1バイト出力 
  int8_t  IsText(char* tfname);                         // ファイルがテキストファイルかチェック
  int16_t readLine(char* str);                          // 1行分読込み
  int8_t  textOut(char* fname, int16_t sline, int16_t ln); // テキストファイルの出力
    
  // ビットマップファイルのロード
  uint8_t loadBitmap(char* fname, uint16_t dst_x, uint16_t dst_y, int16_t x, int16_t y, int16_t w,int16_t  h, uint8_t mode=0);
};
#endif
