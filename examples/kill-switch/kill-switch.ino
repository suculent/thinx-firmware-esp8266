/*
 * THiNX Example expecting a relay connected to D1
 *
 * - Set your own WiFi credentials for development purposes.
 * - When THiNX finalizes checkin, update device status over MQTT.
 * - Use the `Push Configuration` function in website's action menu to trigger pushConfigCallback() [limit 512 bytes so far]
 */

#include <Arduino.h>
#include <ArduinoJson.h>

#define ARDUINO_IDE

#include <THiNXLib.h>

const char *apikey = "";
const char *owner_id = "";
const char *ssid = "THiNX-IoT";
const char *pass = "<enter-your-ssid-password>";

THiNX thx;

void toggle() {
  relayState = !relayState;
  digitalWrite(D1, relayState);        
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

  bool relayState = LOW;
  uint64_t toggleTimeout = 0;

  //
  // Initialization
  //x

  // THiNX::forceHTTP = true; disable HTTPS to speed-up checkin in development

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
  thx.publishStatus("STATUS:RETAINED"); // set MQTT status (unretained)
  });

  // Callbacks can be defined inline
  thx.setMQTTCallback([](String message) {
    Serial.println(message);

    // Convert incoming JSON string to Object
    DynamicJsonBuffer jsonBuffer(512);
    JsonObject& root = jsonBuffer.parseObject(message.c_str());
    String command = root["command"];
  
    if ( !command.success() ) {
      Serial.println(F("No command."));
    } else {        

      // turn relay on
      if (command.indexOf("on") != -1) {
        relayState = HIGH;
        digitalWrite(D1, relayState);        
        return;
      }

      // turn relay off
      if (command.indexOf("off") != -1) {
        relayState = LOW;
        digitalWrite(D1, relayState);        
        return;
      }

      // toggle relay state now
      if (command.indexOf("toggle") != -1) {
        toggle();
        return;
      }

      // toggle relay state after timer
      if (command.indexOf("timer") != -1) {        
        JsonObject& params = root["params"];      
        if ( !params.success() ) {
          Serial.println(F("No params."));
        } else {
          int timeout = params["timeout"];
          toggleTimeout = millis() + timeout * 1000; // will toggle after timeout in seconds
        }        
      }      
  });

}

/* Loop must call the thx.loop() in order to pickup MQTT messages and advance the state machine. */
void loop()
{
  thx.loop();
  
  // Toggle on timeout
  if ((toggleTimeout > 0) && (toggleTimeout < millis()) {
    toggle();
    toggleTimeout = 0;
  }  
}
