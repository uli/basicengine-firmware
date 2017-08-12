// 
// sdbitmap.h
// ビットマップファイル操作クラス
// 2016/06/21 たま吉さん
// 2017/04/30,修正,オーバーフロー対応(unt8_tをuint16_tに変更)
// 2017/06/08,修正,getBitmap()に引数追加、幅指定不具合対応
//

#ifndef __SDBITMAP_H__
#define __SDBITMAP_H__

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

// クラス定義
class sdbitmap {
 
 // メンバ変数の定義
 private:
	char*	 _filename;	          // ビットマップファイル名
	File	 _bmpfile;		      // ファイルオブジェクト
	uint8_t  _sts;			      // ファイルアクセス状態(0:初期 1:オープン中1)
	int16_t  _bmpWidth;		      // 画像幅
	int16_t  _bmpHeight;	      // 画像高さ
	uint32_t _bmpImageoffset;     // Start of image data in file
	uint32_t _rowSize;	  	      // 1ラインのバイトサイズ
	uint32_t _imgSize;			  // 画像部サイズ
	boolean  _flip;               // 画像格納方向
	 
 // メンバ関数の定義
 public:
	sdbitmap();								// コンストラクタ
	void    init();			      		    // 初期化
	void    setFilename(char* str);		    // ファイルの設定
	uint8_t open();						    // ファイルのオープン
	void	close();					    // ファイルのクローズ
	int16_t getWidth();					    // 画像幅の取得
	int16_t getHeight();					// 画像高さの取得	 
	uint32_t getImageSize();                // 画像部サイズ(バイト)

    uint8_t getByte(uint16_t x, uint16_t y) ;   // 指定位置から8ドット(バイトデータ)取り出し
  
	uint8_t getBitmap(uint8_t*bmp,    		    // ビットマップデータの切り出し取得(高速版)
	  uint16_t x, uint16_t y, 
	  uint16_t w, uint16_t h, uint8_t mode, uint16_t offset=0);

  uint8_t getBitmapEx(uint8_t*bmp,              // ビットマップデータの切り出し取得
    uint16_t x, uint16_t y, 
    uint16_t w, uint16_t h, uint8_t mode);

	uint8_t getBitmap(uint8_t*bmp, uint8_t mode);	// ビットマップデータの取得  

 private:
	uint16_t read16();	// ワードデータ読み込み
	uint32_t read32();	// ロングデータ読み込み
};

#endif
