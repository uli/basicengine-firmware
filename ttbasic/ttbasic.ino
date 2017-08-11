/*
  TOYOSHIKI Tiny BASIC for Arduino
 (C)2012 Tetsuya Suzuki
  2017/03/22 Modified by Tamakichi、for Arduino STM32
 */

#include "ttconfig.h"

void basic();
uint8_t serialMode;

void setup(void){
  Serial.begin(115200);
  Serial1.begin(GPIO_S1_BAUD);

  // USBのジッター低減
/*
  //Serial.end();
  nvic_irq_set_priority(NVIC_USB_HP_CAN_TX, 7);  // USB割り込み優先レベル設定
  nvic_irq_set_priority(NVIC_USB_LP_CAN_RX0,7);  // USB割り込み優先レベル設定
  nvic_irq_set_priority((nvic_irq_num)14,4);
*/
  nvic_irq_set_priority(NVIC_TIMER2,2);

#if defined (__STM32F1__)   
  for(uint8_t tm=0; tm <15 && !Serial; tm++) {
    delay(100);
  }
#endif
  randomSeed(analogRead(PA0));
}

void loop(void){
  basic();
}
