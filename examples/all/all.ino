/*
 * THiNX Example with all features
 *
 * - Set your own WiFi credentials for development purposes.
 * - When THiNX finalizes checkin, update device status over MQTT.
 * - Use the `Push Configuration` function in website's action menu to trigger pushConfigCallback() [limit 512 bytes so far]
 */

#include <Arduino.h>

#define ARDUINO_IDE

#include <THiNXLib.h>

char *apikey    = (const char*) "";
char *owner_id  = (const char*) "";
char *ssid      = (const char*) "THiNX-IoT";
char *pass      = (const char*) "<enter-your-ssid-password>";

THiNX thx;

//
// Example of using injected Environment variables to change WiFi login credentials.
//

void pushConfigCallback (char * config_cstring) {

  // Set MQTT status (unretained)
  thx.publish_status_unretained((const char*)"{ \"status\" : \"push configuration received\"}");

  // Convert incoming JSON string to Object
  DynamicJsonDocument root(512); // tightly enough to fit full OTT response as well
  auto error = deserializeJson(root, config_cstring);

  if ( error ) {
    Serial.println(F("Failed parsing configuration."));
    return;
  }
  // Parse and apply your Environment vars
  const char *ssid = root["configuration"]["THINX_ENV_SSID"];
  const char *pass = root["configuration"]["THINX_ENV_PASS"];
  // password may be empty string
  if ((strlen(ssid) > 2) && (strlen(pass) > 0)) {
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
    ssid = ""; pass = "";
    unsigned long timeout = millis() + 20000;
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

void setup() {

  Serial.begin(115200);

#ifdef __DEBUG__
  while (!Serial); // wait for debug console connection
#endif

  // If you need to inject WiFi credentials once...
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  //
  // Static pre-configuration
  //

  THiNX::accessPointName = "THiNX-AP";
  THiNX::accessPointPassword = "PASSWORD";

  //
  // Initialization
  //

  //THiNX::accessPointName = "THiNX-AP";
  //THiNX::accessPointPassword = "<enter-ap-mode-password>";
  THiNX::forceHTTP = true; // disable HTTPS to speed-up checkin in development

  thx = THiNX(apikey, owner_id);

  //
  // App and version identification
  //

  // Override versioning with your own app before checkin
  thx.thinx_firmware_version = "ESP8266-THiNX-App-1.0.0";
  thx.thinx_firmware_version_short = "1.0.0";

  //
  // Callbacks
  //

  // Called after library gets connected and registered.
  thx.setFinalizeCallback([]{
    Serial.println("*INO: Finalize callback called.");
  thx.publish_status_unretained((const char*)"{ \"status\" : \"Hello, world!\" }"); // set MQTT status
  });

  // Called when new configuration is pushed OTA
  thx.setPushConfigCallback(pushConfigCallback);

  // Callbacks can be defined inline
  thx.setMQTTCallback([](byte * message) {
    Serial.println((char*)message);
  });

}

/* Loop must call the thx.loop() in order to pickup MQTT messages and advance the state machine. */
void loop()
{
  thx.loop();
}
