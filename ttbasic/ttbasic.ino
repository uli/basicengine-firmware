/*
  TOYOSHIKI Tiny BASIC for Arduino
 (C)2012 Tetsuya Suzuki
  2017/03/22 Modified by Tamakichi、for Arduino STM32
 */

//
// 2017/03/22 修正, Arduino STM32、フルスクリーン対応 v0.1 by たま吉さん
// 

void basic(void);

void setup(void){
  // put your setup code here, to run once:
  Serial.begin(115200);
  // USBのジッター低減
  nvic_irq_set_priority(NVIC_USB_HP_CAN_TX, 7);  // USB割り込み優先レベル設定
  nvic_irq_set_priority(NVIC_USB_LP_CAN_RX0,7);  // USB割り込み優先レベル設定
  nvic_irq_set_priority((nvic_irq_num)14,4);
  nvic_irq_set_priority(NVIC_TIMER2,2);
#if defined (__STM32F1__)   
  //while (!Serial.isConnected()) delay(100);
  //delay(3000);
#endif
  //randomSeed(analogRead(0));
}

void loop(void){
  // put your main code here, to run repeatedly:
  basic();
}
