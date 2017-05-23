#include <TTVout.h>
#include <fontALL.h>

TTVout TV;

void setup()  {
  Serial.begin(); // USBシリアル利用
  delay(3000);
  
  TV.begin();
  TV.select_font(font6x8);
  TV.println("Serial Terminal");
  TV.println("-- Version 0.1 --");
}

void loop() {
  if (Serial.available()) {
    TV.print((char)Serial.read());
  }
}
