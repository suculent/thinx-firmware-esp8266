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

const char *ssid_1 = "SSID-1";
const char *pass_1 = "password-1";

const char *ssid_2 = "SSID-2";
const char *pass_2 = "password-2";

int ledstate;

const int relay_pin = D1;
const int LED_PIN = D4;
bool relay_state = false;
long toggle_delay = -1;

THiNX thx;

#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

//

void scan_and_connect_wifi() {
  // If you need to inject WiFi credentials once...
  WiFi.mode(WIFI_STA);

  unsigned long timeout; // wifi connection timeout(s)
  int wifi_status = WiFi.waitForConnectResult();
  
  WiFi.begin(ssid_1, pass_1);
  delay(2000);

  if (wifi_status != WL_CONNECTED) {    
    Serial.println(F("Waiting for WiFi connection..."));
    
    timeout = millis() + 5000;
    while (millis() < timeout) {
      ledstate = !ledstate;
      digitalWrite(LED_PIN, ledstate);
      delay(500);
      Serial.print(".");
    }
  }

  wifi_status = WiFi.waitForConnectResult();

  if (wifi_status != WL_CONNECTED) {
    WiFi.begin(ssid_2, pass_2);
    delay(2000);
  
    timeout = millis() + 5000;
    while (millis() < timeout) {
      ledstate = !ledstate;
      digitalWrite(LED_PIN, ledstate);
      delay(500);
      Serial.print(".");
    }
  }

  wifi_status = WiFi.waitForConnectResult();

  if (wifi_status != WL_CONNECTED) {
    Serial.println("WiFi Failed. Rebooting...");
    Serial.flush();
    ESP.reset();
  }
 
}

void setup() {  

  Serial.begin(230400);

#ifdef __DEBUG__
  while (!Serial); // wait for debug console connection
#endif

  scan_and_connect_wifi();

  //
  // Static pre-configuration
  //

  THiNX::accessPointName = "THiNX-AP";
  THiNX::accessPointPassword = "PASSWORD";
  THiNX::forceHTTP = true; // disable HTTPS for faster checkins, enable for production security
  
  //
  // Initialization
  //

  thx = THiNX(apikey, owner_id);

  //
  // App and version identification
  //

  // Override versioning with your own app before checkin
  thx.thinx_firmware_version = "ESP8266-THiNX-Relay-0.0.1";
  thx.thinx_firmware_version_short = "0.0.1";

  //
  // Callbacks
  //

  // Called after library gets connected and registered.
  thx.setFinalizeCallback([] {
    Serial.println("THX: Finalize callback called.");

    thx.publishStatus("{ \"status\" : \"server-ready\" }"); // set MQTT status (unretained)

    server.on("/", []() {
      Serial.println("\nHTTP root requested...");
      server.send(200, "text/plain", "Welcome to Terminator D1");
    });

    server.on("/help", []() {
      Serial.println("\nHTTP help requested...");
      server.send(200, "text/plain", "[on, off, toggle] where toggle has default 5 seconds and starts with terminated state");
    });

    server.on("/on", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP on requested...");
      terminator_on();
      server.send(200, "text/plain", "Toggled ON.");
    });

    server.on("/off", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP off requested...");
      terminator_off();
      server.send(200, "text/plain", "Toggled OFF.");
    });

    server.on("/toggle", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP toggle requested...");
      terminator_on();
      set_toggle_delay(5000);      
      server.send(200, "text/plain", "Toggled OFF/ON after 5 seconds.");
    });

    server.onNotFound([](void){
      server.send(404, "text/plain", "No go.");
    });
    

    server.begin();

    Serial.println("SETUP: HTTP server started");
    digitalWrite(LED_PIN, HIGH);
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
      terminator_on();
    }

    if (message.indexOf("off") != -1) {
      Serial.println("MQTT: Relay OFF");
      terminator_off();
    }

    if (message.indexOf("delay") != -1) {
      int param = root["param"];
      Serial.print("MQTT: Relay toggle with param ");
      Serial.println(param);
      terminator_on();
      toggle_after(param);
    }

  });

  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, LOW);

}


// Business Logic

/* User wants to change current status to ON. */

void terminator_on() {
  Serial.println("BUS: Terminator ON...");
  set_state(true);
}

/* User wants to change current status to OFF. */
void terminator_off() {
  Serial.println("BUS: Terminator OFF...");
  set_state(false);
}

/* User wants to flip the current status after a timeout. */
void toggle_after(int seconds) {
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
    digitalWrite(relay_pin, relay_state); // ! because connected to NC port of relay    
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
  if ( (toggle_delay > 0) && (toggle_delay < millis()) ) {
    Serial.println("PROC: Toggle delay expired. Switching...");
    toggle_delay = -1;
    terminator_off();
    Serial.print("PROC: Result state: ");
    Serial.println(relay_state);
  } else {
    set_state(relay_state);
  }
}

/* Switches current relay state. Change will occur on process_state() in main loop. */
void relay_toggle() {
  Serial.print("TOGGLE: Toggling state: ");
  Serial.println(relay_state);
  set_state(!relay_state);
  toggle_delay = -1;
  Serial.print("TOGGLE: Final state: ");
  Serial.println(relay_state);
}

/* Loop must call the thx.loop() in order to pickup MQTT messages and advance the state machine. */
void loop(void)
{
  process_state();
  thx.loop();
  server.handleClient();
  delay(100);
}
