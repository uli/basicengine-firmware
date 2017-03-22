## 豊四季タイニーBASIC for Arduino Stm32 修正版 V0.1

本プログラムは、下記オリジナル版の修正版です.  
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
