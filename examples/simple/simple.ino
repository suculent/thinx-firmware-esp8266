/* Minimal THiNX firmware for ESP 8266 (Arduino) */

#include <dummy.h>
#include "Arduino.h"

#define __DEBUG__ // wait for serial port on boot

// 1. Include
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

  // THiNX::forceHTTP = true; disable HTTPS to speed-up checkin in development

  // 3. initialize with API Key and Owner ID
  thx = THiNX("23a80bf9d8e1ce4d5c0be292b26bf3a935824d19a85344d238089722c6be335a", "cedc16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12");
}

void loop()
{
  // 4. Runloop: waits for WiFI, registers, checks MQTT, reconnects, updates, processes changes...
  thx.loop();
}
