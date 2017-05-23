#include <TTVout.h>
#include <fontALL.h>

TTVout TV;
void setup() {
  TV.begin(SC_224x108);
  TV.select_font(font6x8);
  TV.println("tv.tone() sample");

  TV.tone(440, 100);
  TV.tone(880,100);  
  delay(500);

  TV.tone(330, 150);
  delay(150);
  TV.tone(294, 150);
  delay(150);
  TV.tone(262, 300);
  delay(300);
  TV.tone(294, 300);
  delay(300);
  TV.tone(330, 300);
  delay(300);
  TV.tone(440, 300);
  delay(300);
  TV.tone(392, 150);
  delay(300);
  TV.tone(330, 150);
  delay(150);
  TV.tone(330, 150);
  delay(150);
  TV.tone(330, 300);
  delay(300);
  TV.tone(294, 150);
  delay(150);
  TV.tone(262, 150);
  delay(150);
  TV.tone(294, 300);
}

void loop() {

}
