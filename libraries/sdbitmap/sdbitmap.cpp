//
// sdbitmap.cpp
// ビットマップファイル操作クラス
// 2016/06/21 たま吉さん
// 2017/04/30,修正,オーバーフロー対応(unt8_tをuint16_tに変更)
// 2017/06/08,修正,getBitmap()に引数追加、幅指定不具合対応
//

#include "sdbitmap.h"
#define DEBUG_BMPLOAD 0

//
// コンストラクタ
//

sdbitmap::sdbitmap() {
	init();
}

//
// 初期化
//
void sdbitmap::init() {
	_filename = NULL;
	_sts = 0;
}

//
// ファイル名設定
//
void sdbitmap::setFilename(char* str) {
	_filename = str;	
}

//
// ワードデータ読み込み
//
uint16_t sdbitmap::read16() {
  uint16_t result;
  ((uint8_t *)&result)[0] = _bmpfile.read(); // LSB
  ((uint8_t *)&result)[1] = _bmpfile.read(); // MSB
  return result;
}

//
// ロングデータ読み込み
//
uint32_t sdbitmap::read32() {
  uint32_t result;
  ((uint8_t *)&result)[0] = _bmpfile.read(); // LSB
  ((uint8_t *)&result)[1] = _bmpfile.read();
  ((uint8_t *)&result)[2] = _bmpfile.read();
  ((uint8_t *)&result)[3] = _bmpfile.read(); // MSB
  return result;
}


//
// ビットマップファイルのオープン 
// 引数なし
// 戻り値	0:正常終了
//			1:オープン失敗
//			2:フォーマットエラー
// 
uint8_t	sdbitmap::open() {
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t tmp_val;

  _flip    = true;      		  // BMP is stored bottom-to-top

  // ファイルオープン

#if DEBUG_BMPLOAD == 1
  Serial.println();
  Serial.print("Loading image '"); Serial.print(_filename);  Serial.println('\'');
#endif
 
  // Open requested file on SD card
  if ((_bmpfile = SD.open(_filename)) == NULL) {

#if DEBUG_BMPLOAD == 1
    Serial.print("File not found");
#endif

    return 1;	// オープン失敗
  }

  // [ファイルヘッダ] (14バイト)
  //   ｱﾄﾞﾚｽ (ｻｲｽﾞ)  名称  内容
  //   0x0000　(2)  bfType  ファイルタイプ　通常は'BM'
  //   0x0002　(4)  bfSize  ファイルサイズ (byte)
  //   0x0006　(2)  bfReserved1 予約領域　常に 0
  //   0x0008　(2)  bfReserved2 予約領域　常に 0
  //   0x000A　(4)  bfOffBits ファイル先頭から画像データまでのオフセット (byte)
  //

  if(read16() != 0x4D42) 			  // BMPシグニチャチェック(2バイト)
	return 2;						  // フォーマットエラー
	
  tmp_val = read32();			   	  // ファイルサイズの取得(4バイト)
#if DEBUG_BMPLOAD == 1    
    Serial.print("File size: "); Serial.println(tmp_val);
#endif
    tmp_val = read32();				  // 予約領域の取得(4バイト)
    _bmpImageoffset = read32();		  // オフセット値の取得(4バイト)
#if DEBUG_BMPLOAD == 1    
    Serial.print("Image Offset: "); Serial.println(_bmpImageoffset, DEC);
#endif

  // [情報ヘッダ] (40バイト)
  //   0x000E　(4)  bcSize          ヘッダサイズ 40
  //   0x0012　(4)  bcWidth         画像の幅 (ピクセル)
  //   0x0016　(4)  bcHeight        画像の高さ (ピクセル)
  //   0x001A　(2)  bcPlanes        プレーン数　常に 1
  //   0x001C　(2)  bcBitCount      1画素あたりのデータサイズ
  //   0x001E　(4)  biCompression   圧縮形式 0:BI_RGB(無圧縮) 1:BI_RLE8 2:BI_RLE4 3: BI_BITFIELDS 4:BI_JPEG 5: BI_PNG
  //   0x0022　(4)  biSizeImage     画像データ部のサイズ (byte) 
  //   0x0026　(4)  biXPixPerMeter  横方向解像度
  //   0x002A　(4)  biYPixPerMeter  縦方向解像度
  //   0x002E　(4)  biClrUsed       格納されているパレット数
  //   0x0032　(4)  biCirImportant  重要なパレットのインデックス
  //
    
  tmp_val = read32();		// ヘッダサイズの取得

#if DEBUG_BMPLOAD == 1        
    Serial.print("Header size: "); Serial.println(tmp_val);
#endif

  _bmpWidth  = read32();	// 画像幅の取得
  _bmpHeight = read32();	// 画像高さの取得
   
  if(read16() != 1)         // プレーン数の取得（必ず１である必要がある）
 	return 2;
 
  bmpDepth = read16();  // 1ピクセル当たりのビット数
#if DEBUG_BMPLOAD == 1        
    Serial.print("Bit Depth: "); Serial.println(bmpDepth);
#endif

  if (bmpDepth != 1)	// 2値(白,黒)
  	return 2;			// 形式エラー

  if(read32() != 0 )  	// 圧縮形式：無圧縮か？
  	return 2;			// 形式エラー

  // １ラインのバイト数
  _rowSize = (((_bmpWidth+7)>>3) + 3) & ~3;
  if(_bmpHeight < 0) {
    _bmpHeight = - _bmpHeight;
    _flip      = false;
  }

  // 画像部サイズの取得
//  _imgSize = read32();
  _imgSize = _rowSize*_bmpHeight;
	
#if DEBUG_BMPLOAD == 1    
    Serial.print("row Size: "); Serial.println(_rowSize);
    Serial.print("flip: "); Serial.println(_flip);
    Serial.print("Image size: "); Serial.print(_bmpWidth); Serial.print('x'); Serial.println(_bmpHeight);
#endif
  return 0;
}

//
// ファイルのクローズ
//
void sdbitmap::close() {
  _bmpfile.close();
}

//
// 画像幅の取得
//
int16_t sdbitmap::getWidth() {
	return _bmpWidth;
}

//
// 画像高さの取得
//
int16_t sdbitmap::getHeight() {
	return _bmpHeight;
}

// 画像部サイズ(バイト)
uint32_t sdbitmap::getImageSize() {
	return _imgSize;
}

//
// ビットマップデータの取得
// 引数
//  bmp : データ格納アドレス
//  mode: 0:通常 1:反転
// 戻り値
//  0:     正常終了
//  0以外  異常終了
//
uint8_t sdbitmap::getBitmap(uint8_t*bmp, uint8_t mode) {
  return getBitmap(bmp, 0, 0, _bmpWidth, _bmpHeight, mode);
}


//
// ビットマップデータの切り出し取得
// 引数
//  bmp : データ格納アドレス
//  x   : 取り出し位置：横 (8の倍数であること）
//  y   : 取り出し位置：縦
//  w   : 取り出し幅 (8の倍数であること）
//  h   : 取り出し高さ
//  mode: 0:通常 1:反転
//  offset: 次のラインの位置移動用のオフセット(デフォルト=0)
// 戻り値
//  0:     正常終了
//  0以外  異常終了
// 
uint8_t sdbitmap::getBitmap(uint8_t*bmp, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t mode,uint16_t offset) {
  uint32_t pos = 0;
  uint16_t ptr = 0;

  if (x+w > _bmpWidth) {
	 w = _bmpWidth - x;
  }

  if (y+h > _bmpHeight) {
	 h = _bmpHeight - y;
  }
  uint16_t bx = x>>3;
  uint16_t bw = (w+7)>>3;
  uint8_t bit_w = w & 7;
	
  if (y + h > _bmpHeight)
    return -1;
  if ( ((x + w)>>3) > _rowSize)
    return 1;

  for ( uint16_t row = y; row < y+h ; row++ ) { // ラインループ
    if(_flip) 
      pos = _bmpImageoffset + (_bmpHeight - 1 - row) * _rowSize + bx;
    else
      pos = _bmpImageoffset + row * _rowSize + bx;
      
    // ファイルのシーク  
    if( _bmpfile.position() != pos ) // Need seek?
      _bmpfile.seek(pos);

    // 1ライン分のデータの取得
    for ( uint16_t col = bx ; col < bx+bw ; col++ ) {
      bmp[ptr++] =  mode ? ~_bmpfile.read(): _bmpfile.read();
    }
  	
    if (bit_w && ptr) // 8の倍数でない
      bmp[ptr-1] &= 0xff<<(8-bit_w);
  	if(offset) ptr+=offset-bw;
  }
  return 0;
}

//
// 指定位置から8ビット(1バイト)取り出し
//
uint8_t sdbitmap::getByte(uint16_t x, uint16_t y) {
  uint8_t d1,d2;
  uint32_t pos = 0;
  uint16_t bx = x>>3;        // 取り出しバイト位置：横
  uint8_t bit_x = x & 7;     // ビット位置

  if (bx >= _rowSize || y >= _bmpHeight ) 
    return 0;

  if(_flip) 
    pos = _bmpImageoffset + (_bmpHeight - 1 - y) * _rowSize + bx;
  else
    pos = _bmpImageoffset + y * _rowSize + bx;

    // ファイルのシーク  
    if( _bmpfile.position() != pos )
      _bmpfile.seek(pos);

  d1 = _bmpfile.read();
  if (bit_x) {
    // 8の倍数でない
    d1<<=bit_x;
    d2 = (bx+1 >= _rowSize) ? 0:_bmpfile.read()>>(8-bit_x);
  } else {
    // 8の倍数
    d2 = 0;
  }
  return d1|d2;
}

//
// ビットマップデータの切り出し取得
// 引数
//  bmp : データ格納アドレス
//  x   : 取り出し位置：横
//  y   : 取り出し位置：縦
//  w   : 取り出し幅 
//  h   : 取り出し高さ
//  mode: 0:通常 1:反転
// 戻り値
//  0:     正常終了
//  0以外  異常終了
// 
uint8_t sdbitmap::getBitmapEx(uint8_t*bmp, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t mode) {
  // ラインループ
  uint16_t ptr = 0;
  uint8_t d;
  uint8_t bit_w = w % 8;
  for (uint16_t row = y; row < y+h ; row++ ) { 
    for (uint16_t col = x; col < x + w ; col += 8 ) {
      bmp[ptr++] = mode? ~getByte(col,row):getByte(col, row);      
    }
    if (bit_w && w > 8) // 右端の端数ビットの補正
      bmp[ptr-1] &= 0xff<<(8-bit_w);
  }  
  return 0;
}
