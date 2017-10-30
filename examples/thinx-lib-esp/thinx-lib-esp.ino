/* THiNX firmware for ESP 8266 (Arduino) */

#include <dummy.h>
#include "Arduino.h"

#define __DEBUG__ // wait for serial port on boot

// 1. Include the THiNXLib
#include <THiNXLib.h>

// 2. Declare
THiNX thx;

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
  // 4. initialize with API Key and Owner ID
  thx = THiNX("71679ca646c63d234e957e37e4f4069bf4eed14afca4569a0c74abf503076732", "cedc16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12"); 
}

void loop()
{
  // 4. Wait for WiFI, register, check MQTT, reconnect, update, process changes...
  thx.loop();
}
