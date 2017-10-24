/* THiNX firmware for ESP 8266 (Arduino) */

#include <dummy.h>
#include "Arduino.h"

#define __DEBUG__ // wait for serial port on boot

// 1. Include the THiNXLib
#include <THiNXLib.h>

// 2. Declare
THiNX thx;

// 3. Enter your API Key
const char* apikey = "71679ca646c63d234e957e37e4f4069bf4eed14afca4569a0c74abf503076732";

void setup() {
  Serial.begin(115200);
#ifdef __DEBUG__
  while (!Serial);
  delay(5000);
  Serial.setDebugOutput(true);
#endif

  // 4. Initialize
  Serial.println("\n");
  Serial.println("*TH: Initializing in 5 seconds...");
  delay(5000);
  thx = THiNX(apikey); // 4. initialize with API Key
}

void loop()
{
  delay(10000);
  // 5. Waits for WiFI, register, check MQTT, reconnect, update...
  thx.loop();
  Serial.printf("Free size: %u\n", ESP.getFreeSketchSpace());
}
