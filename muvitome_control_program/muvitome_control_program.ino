8 /#include <Wire.h>
#include "Adafruit_TCS34725.h"
const int relay1 = 3;   //Arduino pin that triggers relay #1 (aka relay 3)
const int relay2 = 2;   //Arduino pin that triggers relay #2 (aka relay 4)
#define commonAnode true

int light_on = 1;   //These variables track the status of the indicator light
int light_off = 1;  //We only want to send a signal to the serial port when the light changes, 
                    //we don't want to continuously update the serial port with the status of the light, 
                    //or else the buffer will fill up and the MUVitome will get an outdated status.
                    //We start with both variables set to true so the program first takes a mosaic before making a cut when automatic mode is activated
                    
// our RGB -> eye-recognized gamma color
byte gammatable[256];

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

int val=0;// initialize variable val

void setup() {
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  
  retractActuator(); //make sure the linear actuator is fully retracted on startup
  delay(250);
  stopActuator();

  Serial.begin(9600); //The serial port is set to 9600 Baud, this could be changed, but I dont't know why you would need to
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
float red, green, blue; //we track the amount of red, green, and blue light

tcs.setInterrupt(true);  // turn off sensor LED. The  flora comes with a white LED light, which we want to turn off

  delay(60);  // takes 50ms to read

  tcs.getRGB(&red, &green, &blue); //get the amount of red, green, and blue light

if (red > 200 && green < 50 && blue < 50 && light_on == 0){ //The microtome light is ON (high levels of R, low levels of G and B)
  Serial.print(1); //send a 1 to the serial port
  light_on = 1;    //update the light_on variable
}

if (red < 200 && light_on == 1){ //If the light is OFF, we send a 0 to the serial port
  Serial.print(0);
  light_on = 0;
}

if (Serial.available()){         //When MUVitome is ready to take a cut, it sends a "c" to the arduino via the serial port 
  char ch = Serial.read();
  if (ch = 'c'){                 //If we read a "c", we extend/retract the actuator twice to press the start/stop button
    extendActuator();
    delay(100);                  //IMPORTANT: The delay() is how long we drive the actuator forward/backwards. 
    stopActuator();              //I found 100ms to be the right amount when the retracted actuator head is flush with the bottom of its holder,
                                 //but this would need to be changed if the mounting system is changed
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

  if (ch = 'p'){                //If we send a "p" to the arduino, the actuator presses the start/stop button once. 
    extendActuator();           //This will not trigger a cut
    delay(100);
    stopActuator();

    retractActuator();
    delay(100);
    stopActuator();
  }
}

}
void extendActuator() {  //extends the actuator
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, LOW);
}

void retractActuator() { //retracts the actuator
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, HIGH);
}

void stopActuator() {    //stops the actuator
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
}