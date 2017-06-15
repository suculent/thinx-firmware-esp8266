/* OTA enabled firmware for Wemos D1 (ESP 8266, Arduino) */

// version 1.5.57 (aligned with API build 1015)

#include "Arduino.h"

#include "Thinx.h"
#include "./thinx-lib-esp8266-arduinoc/src/thinx-lib-esp.h"

THiNX* thx = NULL;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.setDebugOutput(true);

#ifdef __DEBUG_WIFI__
  WiFi.begin("page42.showflake.czf", "quarantine");
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

  // AHA! thx needs to export its mqtt_client

  //if (thx_mqtt_client.connected()) {
  //  thx_mqtt_client.publish(thinx_mqtt_channel().c_str(), thx_connected_response.c_str());
  //}
}
