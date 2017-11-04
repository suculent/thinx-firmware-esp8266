#include "Arduino.h"
#include <THiNXLib.h>

// #ifdef __DEBUG__ // uncomment to force injection of custom password/ssid below

THiNX thx;

/* Called after library gets connected and registered */
void finalizeCallback () {
  Serial.println("*INO: Finalize callback called.");
  ESP.deepSleep(3e9);
}

/* Example of using Environment variables */
void pushConfigCallback (String config) {

  // Convert incoming JSON string to Object
  DynamicJsonBuffer jsonBuffer(512);
  JsonObject& root = jsonBuffer.parseObject(config.c_str());
  JsonObject& configuration = root["configuration"];

  if ( !configuration.success() ) {
    Serial.println(F("Failed parsing configuration."));
  } else {

    // Parse and apply your Environment vars
    const char *ssid = configuration["THINX_ENV_SSID"];
    const char *pass = configuration["THINX_ENV_PASS"];

    // password may be empty string
    if ((strlen(ssid) > 2) && (strlen(pass) > 0)) {
      WiFi.disconnect();
      WiFi.begin(ssid, pass);
      long timeout = millis() + 20000;
      Serial.println("Attempting WiFi migration...");
      while (WiFi.status() != WL_CONNECTED) {
        yield();
        if (millis() > timeout) break;
      }
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi migration failed."); // TODO: Notify using publish() to device status channel
      } else {
        Serial.println("WiFi migration successful."); // TODO: Notify using publish() to device status channel
      }
    }
  }
}

void setup() {

  Serial.begin(115200);

#ifdef __DEBUG__
  while (!Serial); // wait for debug console connection
  WiFi.begin("THiNX-IoT+", "<enter-your-ssid-password>");
#endif

   // Enter API Key and Owner ID
  thx = THiNX("71679ca646c63d234e957e37e4f4069bf4eed14afca4569a0c74abf503076732", "cedc16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12");
  thx.setFinalizeCallback(finalizeCallback);
  thx.setPushConfigCallback(pushConfigCallback);
}

/* Loop must call the thx.loop() in order to pickup MQTT messages and advance the state machine. */
void loop()
{
  thx.loop();
}
