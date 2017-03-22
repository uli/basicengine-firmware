/*
  TOYOSHIKI Tiny BASIC for Arduino
 (C)2012 Tetsuya Suzuki
 */

//
// 2017/03/22 修正, Arduino STM32、フルスクリーン対応 v0.1 by たま吉さん
// 

void basic(void);

void setup(void){
  // put your setup code here, to run once:
  Serial.begin(115200);
#if defined (__STM32F1__)   
  while (!Serial.isConnected()) delay(100);
#endif
  randomSeed(analogRead(0));
}

void loop(void){
  // put your main code here, to run repeatedly:
  basic();
}
