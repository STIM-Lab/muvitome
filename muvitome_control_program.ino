8 /#include <Wire.h>
#include "Adafruit_TCS34725.h"
const int relay1 = 3;   //Arduino pin that triggers relay #1
const int relay2 = 2;   //Arduino pin that triggers relay #2
#define commonAnode true

int light_on = 1;
int light_off = 1;

// our RGB -> eye-recognized gamma color
byte gammatable[256];

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

int val=0;// initialize variable val

void setup() {
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  
  Serial.begin(9600);
  if (tcs.begin()) {
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
  }

  // gamma table helps convert RGB colors to what humans see
  for (int i=0; i<256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;

    if (commonAnode) {
      gammatable[i] = 255 - x;
    } else {
      gammatable[i] = x;
    }
  }
}

void loop()
{
float red, green, blue;

tcs.setInterrupt(true);  // turn off sensor LED

  delay(60);  // takes 50ms to read

  tcs.getRGB(&red, &green, &blue);

if (red > 200 && green < 50 && blue < 50 && light_on == 0){
  Serial.print(1);
  light_on = 1;
}

if (red < 200 && light_on == 1){
  Serial.print(0);
  light_on = 0;
}

if (Serial.available()){// 
  char ch = Serial.read();
  if (ch = 'c'){
    extendActuator();
    delay(100);
    stopActuator();

    retractActuator();
    delay(100);
    stopActuator();

    extendActuator();
    delay(100);
    stopActuator();

    retractActuator();
    delay(100);
    stopActuator();
  }
}

}
void extendActuator() {
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, LOW);
}

void retractActuator() {
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, HIGH);
}

void stopActuator() {
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
}