## 豊四季タイニーBASIC for Arduino 修正版 V0.1

![サンプル画像](./image/top.jpg)

本プログラムは、下記オリジナル版の修正版です.   
Arduino MEGA 2560、Blue Pillボードでの動作を確認しました.  
Arduinoではメモリ不足のため動きません.   
※現状、input文の処理に若干問題あるかもしれません.  

- オリジナル版配布サイト  
 https://github.com/vintagechips/ttbasic_arduino  
 関連情報 [電脳伝説 Vintagechips - 豊四季タイニーBASIC確定版](https://vintagechips.wordpress.com/2015/12/06/%E8%B1%8A%E5%9B%9B%E5%AD%A3%E3%82%BF%E3%82%A4%E3%83%8B%E3%83%BCbasic%E7%A2%BA%E5%AE%9A%E7%89%88/)

**「豊四季タイニーBASIC」**の著作権は開発者の**Tetsuya Suzuki**氏にあります.  
プログラム利用については、オリジナル版の著作権者の配布条件に従うものとします.  
著作権者の同意なしに経済的な利益を得てはいけません.  
この条件のもとで、利用、複写、改編、再配布を認めます.  

**修正内容**
- ラインエディタ部の差し換え  
 オリジナルのラインエディタ部分をフルスリーンテキストエディタに差し換えました.  
 ターミナル上で昔のBASICっぽい編集操作を出来るようにしました.  
- コマンドの追加  
  - CLS ：画面クリア
  - LOCATE：カーソル移動 
  - COLOR： 文字色の指定
  - ATTR：文字装飾の指定
  - WAIT：時間待ち

本スケッチの利用には、  
別途、mcursesライブラリ(https://github.com/ChrisMicro/mcurses) が必要です。  

## フルスリーンテキストエディタの機能
※利用にはTeraTerm用のシリアル接続可能なターミナルソフトが必要です.  

**スクリーンサイス**  
40列ｘ20行  

**利用可能キー**  
- [←][→][↑][↓] ： カーソル移動 カーソルキー  
- [Delete] ：カーソル位置の文字削除  
- [BackSpace]：カーソル前の文字削除と前に移動  
- [HOME]：画面の再表示  
- [INS]：挿入・上書きのトグル切り替え  
- [Enter]：行入力確定  

## 追加コマンドの説明

### 画面クリア
- 書式  
 CLS

- 引数  
 なし  

- 説明  
 画面（ターミナル上の表示）をクリア（表示内容消去）します.  
 カーソルは画面左上の座標(0,0)に移動します.  
 直前にCOLORコマンドで背景色の設定を行っている場合は、背景色でクリアされます。  

### カーソル移動
- 書式  
 LOCATE 横位置, 縦位置  
- 引数  
 横位置: 0～39
 縦位置: 0～19

- 説明  
 文字を表示するカーソルを指定した位置に移動します.  
 指定した座標値が範囲外の場合は範囲の下限または上限に移動します.  

### 文字色の指定

- 書式  
 COLOR 文字色  
 COLOR 文字色, 背景色  

- 引数  
 文字色: 色コード　0～9
 背景色: 色コード　0～9

- 説明  
 文字色の指定を行います.
 指定した色は以降の文字表示に反映されます.  
 利用するターミナルソフトにより正しく表示できない場合があります.  
 画面を[HOME]キーで再表示した場合、色情報は欠落します.  
 
 **色コード**  

 |色コード|意味|
 |:-----:|:-:|
 |0      |黒|
 |1      |赤|
 |2      |緑|
 |3      |茶|
 |4      |青|
 |5      |マゼンタ|
 |6      |シアン|
 |7      |白|
 |8      |黄| 


### 文字表示属性の設定

- 書式  
 ATTR 属性  

- 引数  
 属性: 属性コード　0～4

- 説明  
 文字の表示属性を指定します.  
 指定した表示属性は以降の文字表示に反映されます.  
 利用するターミナルソフトにより正しく表示できない場合があります.  
 画面を[HOME]キーで再表示した場合、色情報は欠落します.  

 **属性コード**  

 |属性コード|意味|
 |:-----:|:-:|
 |0      |標準|
 |1      |下線|
 |2      |反転|
 |3      |ブリンク|
 |4      |ボールド|

### 時間待ち
- 書式  
 WAIT 待ち時間(ミリ秒)  

- 引数  
 待ち時間: 0～32767 (単位ミリ秒)
- 説明  
 引数で指定した時間(ミリ秒単位)、時間待ち(ウェイト)を行います.  
 **(注意)**  
 ウェイト中は[ESC]によるプログラム中断を行うことは出来ません.  


## サンプルプログラム
```
5 CLS
7 LOCATE 5,10
10 COLOR 2; PRINT "hello,",
30 COLOR 6; PRINT "world"
40 COLOR 7,0
50 ATTR 2
55 LOCATE 5,11
60 PRINT "tiny BASIC"
70 ATTR 0
```
## 以降はオリジナルのドキュメントです



TOYOSHIKI Tiny BASIC for Arduino

The code tested in Arduino Uno R3.<br>
Use UART terminal, or temporarily use Arduino IDE serial monitor.

Operation example

&gt; list<br>
10 FOR I=2 TO -2 STEP -1; GOSUB 100; NEXT I<br>
20 STOP<br>
100 REM Subroutine<br>
110 PRINT ABS(I); RETURN

OK<br>
&gt;run<br>
2<br>
1<br>
0<br>
1<br>
2

OK<br>
&gt;

The grammar is the same as<br>
PALO ALTO TinyBASIC by Li-Chen Wang<br>
Except 3 point to show below.

(1)The contracted form of the description is invalid.

(2)Force abort key<br>
PALO ALTO TinyBASIC -> [Ctrl]+[C]<br>
TOYOSHIKI TinyBASIC -> [ESC]<br>
NOTE: Probably, there is no input means in serial monitor.

(3)Other some beyond my expectations.

(C)2012 Tetsuya Suzuki<br>
GNU General Public License
