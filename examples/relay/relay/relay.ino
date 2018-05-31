/*
   THiNX Simple managed relay with HTTP/MQTT-based control

   - Set your own WiFi credentials for development purposes.
   - When THiNX finalizes checkin, update device status over MQTT.

   Known issues:
   - HTTP server does not work :o)
*/

#include <Arduino.h>

#define ARDUINO_IDE

#include <THiNXLib.h>

const char *apikey = "c81a4c9d1e10979bdc9dfe12141c476c05055e096d0c6413fb00a25217715dfd";
const char *owner_id = "cedc16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12";
const char *ssid = "THiNX-IoT";
const char *pass = "<enter-your-ssid-password>";

const int relay_pin = D1;
bool relay_state = false;
long toggle_delay = -1;

THiNX thx;

#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

void handleNotFound();

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
  //x

  // THiNX::forceHTTP = true; disable HTTPS to speed-up checkin in development

  THiNX::forceHTTP = true;

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
  thx.setFinalizeCallback([] {
    Serial.println("*INO: Finalize callback called.");

    thx.publishStatus("{ \"status\" : \"server-ready\" }"); // set MQTT status (unretained)

    server.on("/", []() {
      Serial.println("HTTP root requested...");
      server.send(200, "text/plain", "Supported commands? You wish...");
    });

    server.on("/help", []() {
      Serial.println("HTTP help requested...");
      server.send(200, "text/plain", "[on, off, toggle]");
    });

    server.on("/on", []() {
      Serial.println("HTTP on requested...");
      relay_on();
      server.send(200, "text/plain", "Toggled ON.");
    });

    server.on("/off", []() {
      Serial.println("HTTP off requested...");
      relay_off();
      server.send(200, "text/plain", "Toggled OFF.");
    });

    server.on("/toggle", []() {
      Serial.println("HTTP toggle requested...");
      relay_off();
      set_toggle_delay(5000);
      relay_on();
      server.send(200, "text/plain", "Toggled OFF/ON after 5 seconds.");
    });

    server.onNotFound(handleNotFound);

    server.begin();

    Serial.println("HTTP server started");
    Serial.println("...");
  });

  /*
     Supported messages


    {
    "command" : "on"
    }

    {
    "command" : "off"
    }

    {
    "command" : "delay",
    "param" : 1000
    }

  */

  // Callbacks can be defined inline
  thx.setMQTTCallback([](String message) {

    // Convert incoming JSON string to Object
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.parseObject(message.c_str());
    JsonObject& command = root["command"];

    if (message.indexOf("on") != -1) {
      Serial.println("MQTT: Relay ON");
      relay_on();
    }

    if (message.indexOf("off") != -1) {
      Serial.println("MQTT: Relay OFF");
      relay_off();
    }

    if (message.indexOf("delay") != -1) {
      int param = root["param"];
      Serial.print("MQTT: Relay toggle with param ");
      Serial.println(param);
      relay_toggle_after(param);
    }

  });

  pinMode(relay_pin, OUTPUT);

}

void handleNotFound() {
  server.send(404, "text/plain", "No go.");
}


// Business Logic

/* User wants to change current status to ON. */

void relay_on() {
  Serial.println("BUS: Setting ON...");
  set_state(true);
}

/* User wants to change current status to OFF. */
void relay_off() {
  Serial.println("BUS: Setting OFF...");
  set_state(false);
}

/* User wants to flip the current status after a timeout. */
void relay_toggle_after(int seconds) {
  Serial.println("BUS: Setting toggle_after...");
  set_toggle_delay(seconds);
}

// Technical logic

/* Sets and applies relay state */
void set_state(bool state) {
  if (relay_state != state) {
    Serial.print("TECH: Setting state to ");
    Serial.println(state);
    relay_state = state;
    digitalWrite(relay_pin, relay_state);
  }
}

/* Sets delay based on current time */
void set_toggle_delay(long delay_time) {  
  toggle_delay = millis() + delay_time;
  Serial.print("Setting toggle delay to: ");
  Serial.println(toggle_delay);
}

/* Main loop. Waits until toggle_delay to toggle and reset toggle timeout.
   Otherwise just updates the relay state.
*/
void process_state() {
  if ( (toggle_delay > 0) && (toggle_delay > millis()) ) {
    Serial.println("Toggle delay expired. Switching...");
    toggle_delay = -1;
    relay_toggle();
    Serial.print("Result state: ");
    Serial.println(relay_state);
  } else {
    set_state(relay_state);
  }
}

/* Switches current relay state. Change will occur on process_state() in main loop. */
void relay_toggle() {
  Serial.print("Toggling state: ");
  Serial.println(relay_state);
  set_state(!relay_state);
  Serial.print("Final state: ");
  Serial.println(relay_state);
}

/* Loop must call the thx.loop() in order to pickup MQTT messages and advance the state machine. */
void loop()
{
  process_state();
  thx.loop();
  yield();
  delay(100);
}
