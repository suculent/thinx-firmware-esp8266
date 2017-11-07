/*
 * THiNX Basic Example with all features
 * 
 * - Set your own WiFi credentials for development purposes.
 * - When THiNX finalizes checkin, set your custom status
 * - Use the `Push Configuration` function in website's action menu to trigger pushConfigCallback() [limit 512 bytes so far]
 * - TODO: Run Transformers (JavaScript functions to transform status bytes to human readable form) to the backend
 */

#include <Arduino.h>

#define ARDUINO_IDE

#include <THiNXLib.h>

// Import SSID, PASSWORD and API KEY from file not included in repository
#ifndef THINXCI
#import "settings.h"
#else
// Those are just dummy demo values. Never store those in public repository!
// Save them to settings.h and use .gitigore to keep file on your machine only.
// THiNX CI will be able to add the apikey and owner_id on its own. You can configure
// optinally THINX_ENV_SSID and THINX_ENV_PASS Environment Variables to inject those from CI.

// Add this to your settings.h and ignore this file from repository before committing!
const char *apikey = "";
const char *owner_id = "";
const char *ssid = "THiNX-IoT+";
const char *pass = "<enter-your-ssid-password>";
#endif

THiNX thx;

/* Called after library gets connected and registered */
void finalizeCallback () {
  Serial.println("*INO: Finalize callback called.");
  thx.setStatus("Hello IoT!");
  //ESP.deepSleep(3e9);
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
  WiFi.begin(ssid, pass);
#endif

  thx = THiNX(apikey);
  thx.setStatus("Hello internet!");
  thx.setFinalizeCallback(finalizeCallback);
  thx.setPushConfigCallback(pushConfigCallback);
}

/* Loop must call the thx.loop() in order to pickup MQTT messages and advance the state machine. */
void loop()
{
  thx.loop();
}
