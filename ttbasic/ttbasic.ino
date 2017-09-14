/*
  TOYOSHIKI Tiny BASIC for Arduino
 (C)2012 Tetsuya Suzuki
  2017/03/22 Modified by Tamakichi、for Arduino STM32
 */

//
// 2017/03/22 修正, Arduino STM32、フルスクリーン対応 v0.1 by たま吉さん
// 

extern "C" {
#ifdef ESP8266_NOWIFI
#include <hw/pin_mux_register.h>
#else
#include <eagle_soc.h>
#endif
};
#ifndef ESP8266_NOWIFI
extern "C" {
#include <user_interface.h>
};

void ICACHE_RAM_ATTR shut_up_dog(void)
{
  // Any attempt to disable the software watchdog is subverted by the SDK
  // by re-enabling it as soon as it gets the chance. This is the only
  // way to avoid watchdog resets if you actually want to do anything
  // with your system that is not wireless bullshit.
  system_soft_wdt_feed();
  // See you in one second:
  timer0_write(ESP.getCycleCount() + F_CPU);
}
#endif

void basic(void);

void setup(void){
  // put your setup code here, to run once:
  
  //Serial.begin(115200);
  //Serial.end();
  // USBのジッター低減
  //nvic_irq_set_priority(NVIC_USB_HP_CAN_TX, 7);  // USB割り込み優先レベル設定
  //nvic_irq_set_priority(NVIC_USB_LP_CAN_RX0,7);  // USB割り込み優先レベル設定
  //nvic_irq_set_priority((nvic_irq_num)14,4);
//  nvic_irq_set_priority(NVIC_TIMER2,2);
#if defined (__STM32F1__)   
  //while (!Serial.isConnected()) delay(100);
  //delay(3000);
#endif
  //randomSeed(analogRead(PA0));
#ifndef ESP8266_NOWIFI
  timer0_isr_init();
  timer0_attachInterrupt(&shut_up_dog);
#endif
}

void loop(void){
#ifdef ESP8266_NOWIFI
  ets_wdt_disable();
#endif
  // put your main code here, to run repeatedly:
  basic();
}
