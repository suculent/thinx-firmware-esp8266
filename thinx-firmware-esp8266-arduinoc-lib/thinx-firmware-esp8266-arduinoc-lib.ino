/* OTA enabled firmware for Wemos D1 (ESP 8266, Arduino) */

#include "Arduino.h"

#include "Thinx.h"
#include "./thinx-lib-esp8266-arduinoc/src/thinx-lib-esp.h"

#define __DEBUG_WIFI__ /* use as fallback when device gets stucked with incorrect WiFi configuration, overwrites Flash in ESP */

THiNX* thx = NULL;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.setDebugOutput(true);

#ifdef __DEBUG_WIFI__
  WiFi.begin("HAVANA", "1234567890");
#endif

  Serial.println("Setup completed.");
}

void loop()
{
  if (thx == NULL) {
    thx = new THiNX(thinx_api_key); // why do we have to call it all over? MQTT callback?
  }

  delay(10000);
  Serial.println(".");

  if (thx->mqtt_client->connected()) {
    thx->mqtt_client->publish(thx->thinx_mqtt_channel().c_str(), thx->thx_connected_response.c_str());
  }
}
