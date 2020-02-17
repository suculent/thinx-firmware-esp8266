/*
   THiNX Simple managed relay with HTTP/MQTT-based control

   - Set your own WiFi credentials for development purposes.
   - When THiNX finalizes checkin, update device status over MQTT.

   Known issues:
   - Issues appeared when responding to many messages at once.
*/

// Example MQTT commands:
// toggle port:
// { "command": { "intent": "0", "port": 1 }}
// get status without change:
// { "command": { "intent": "status" }}

// On Lolin, use Wemos D1 R2 & mini board and reset just before uploading, no need to press flash.

#include <Arduino.h>

#ifndef D4
#define D4 (4)
#endif
#ifndef D5
#define D5 (5)
#endif
#ifndef D6
#define D6 (6)
#endif
#ifndef D7
#define D7 (7)
#endif
#ifndef D8
#define D8 (8)
#endif

// send a notice to THiNX builder, may deprecate with thinx.yml
#define ARDUINO_IDE

#include <THiNXLib.h>
#include <ESP8266WebServer.h>

#define STARTUP_STATE 0 // on power-on, all relays should switch to this state (0 = OFF)
#define ENABLE_HTTP_SERVER // might pose security risk or MITM attack

char *apikey = "c81a4c9d1e10979bdc9dfe12141c476c05055e096d0c6413fb00a25217715dfd";
char *owner_id = "cedc16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12";

// Primary SSID
char *ssid_1 = "THiNX-IoT";
char *pass_1 = "<enter-your-ssid-password>";

// Secondary SSID, tried if first fails, poses security risk...
char *ssid_2 = "VINARNA";
char *pass_2 = "najednicku";

const int LED_PIN = D4;
int ledstate;
long toggle_delay = -1;

bool relay_states[4]; // state of relays, source of truth
int  relay_pins[4];   // pin mapping

THiNX thx;
ESP8266WebServer server(80);

//
// This is intentionally unusual design pattern, where one WiFi is default and other is fallback.
// Poses a security risk, but can be used in case where device restarts after WiFi
// connection fail on order to reconnect to backup network:
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

char status_json[128] = {0};
char * status_template = "{ \"relay-states\" : [ %i, %i, %i, %i ] }";
char * error_template = "{ \"error\" : \"%s\" }";

/* Sets and applies relay state */
void report_status() {
  sprintf(status_json, status_template, relay_states[0], relay_states[1], relay_states[2], relay_states[3]);
  thx.publish_status(status_json, false);
}

void report_error(char* reason) {
  sprintf(status_json, error_template, reason);
  thx.publish_status(status_json, false);
}


/* Sets and applies relay state */
void set_state(int index, bool state) {
  if (relay_states[index] != state) {
    Serial.print("TECH: Setting relay ");
    Serial.print(index);
    Serial.print(" state to ");
    Serial.println(state);
    relay_states[index] = state;
    digitalWrite(relay_pins[index], !relay_states[index]);
    report_status(); // send changed state to MQTT immediately
  }
}


void start_http_server() {

    server.on("/", []() {
      if (server.method() == HTTP_POST) {
        // Sample threat: Processing message: volatile.authentication_failed=true&volatile.login=true&webvars.username=%24VERSION&webvars.password=sawmill_detect&submit=Login
        // Should accept only json, starting with '{'
        if (server.arg("plain").length() > 64) {
          // Security/availability: ignore messages longer than longest possible commands to prevent DoS crashes and memory leaks
          Serial.println("Ignoring long message.");
          return;
        }
        // Security/integrity: we do accept only JSON
        if (server.arg("plain").indexOf("{") == 0) {
          process_message(server.arg("plain"));
        } else {
          Serial.println("Ignoring non-JSON message."); // Security measure
        }
      } else {
        server.send(200, "text/plain", "Welcome to Terminator D1");
        // Security/logging: get more details in case of unusual intrusion attempts
        for (int a = 0; a < server.args(); a++) {
          Serial.print(server.argName(a));
          Serial.print(" : ");
          Serial.println(server.arg(a));
        }
      }
    });

    server.on("/help", []() {
      Serial.println("\nHTTP help requested...");
      server.send(200, "text/plain", "[on, off, toggle] where toggle has default 5 seconds and starts with terminated state");
    });


    server.on("/1/on", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP on 1 requested...");
      set_state(0, true);
      server.send(200, "text/plain", "{ \"port\": 0, \"state\": 1 }");
    });

    server.on("/2/on", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP on 2 requested...");
      set_state(1, true);
      server.send(200, "text/plain", "Toggled ON.");
    });

    server.on("/3/on", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP on 3 requested...");
      set_state(2, true);
      server.send(200, "text/plain", "Toggled ON.");
    });

    server.on("/4/on", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP on 4 requested...");
      set_state(3, true);
      server.send(200, "text/plain", "Toggled ON.");
    });

    server.on("/1/off", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP off 1 requested...");
      set_state(0, false);
      server.send(200, "text/plain", "Toggled ON.");
    });

    server.on("/2/off", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP off 2 requested...");
      set_state(1, false);
      server.send(200, "text/plain", "Toggled ON.");
    });

    server.on("/3/off", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP off 3 requested...");
      set_state(2, false);
      server.send(200, "text/plain", "Toggled ON.");
    });

    server.on("/4/off", []() {
      if (server.method() != HTTP_GET) return;
      Serial.println("\nHTTP off 4 requested...");
      set_state(3, false);
      server.send(200, "text/plain", "Toggled ON.");
    });

    server.onNotFound([](void){
      server.send(404, "text/plain", "No go.");
    });

    server.begin();

    Serial.print("HTTP server started at IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_PIN, HIGH);
}

void setup_relays() {
  // relay_pins = D4 == LED !
  relay_pins[0] = D5;
  relay_pins[1] = D6;
  relay_pins[2] = D7;
  relay_pins[3] = D8;

  for (int relay = 0; relay < 4; ++relay) {
    pinMode(relay_pins[relay], OUTPUT);
    delay(10);
    set_state(relay, !STARTUP_STATE);
  }
}

void process_message(String message) {

  // Example message:  { "command": { "intent": "off", "port": 0 }}

  Serial.print("Processing message: "); Serial.println(message);

  // Convert incoming JSON string to Object
  DynamicJsonDocument root(256);
  auto error = deserializeJson(root, message.c_str());
  if (error) {
    return;
  }

  int port = root["command"]["port"];
  String intent = root["command"]["intent"];


  // Status requestStarting server..

  if (intent.indexOf("status") != -1) {
    Serial.println("MQTT: Status requested.");
    report_status();
    return;
  };

  // Intent with port parser

  if (port > 3) {
    Serial.println("MQTT: Incoming port out of range.");
    report_error("port out of range");
    return;
  }

  if ((intent.indexOf("on") != -1) || (intent.indexOf("1") != -1)) {
    Serial.println("MQTT: Relay ON");
    set_state(port, true);
    return;
  };

  if ((intent.indexOf("off") != -1) || (intent.indexOf("0") != -1)) {
    Serial.println("MQTT: Relay OFF");
    set_state(port, false);
    return;
  };
}

void setup() {


  //
  // App and version identification
  //



  //
  // OUTPUT PIN Configuration
  //

  setup_relays();

  //
  // Serial and WiFi
  //

  Serial.begin(230400);

#ifdef __DEBUG__
  while (!Serial); // wait for debug console connection
#endif

  scan_and_connect_wifi();

#ifdef ENABLE_HTTP_SERVER
  start_http_server();
#endif

  //
  // THiNX
  //

  THiNX::accessPointName = "THiNX-AP";
  THiNX::accessPointPassword = "PASSWORD";
  THiNX::forceHTTP = true; // disable HTTPS for faster checkins, enable for production security

  /* with https:

  HTTP server started at IP: 192.168.1.76

  *TH: THiNXLib rev. 230 version: 2.5.230
  cloud.thinx.esp8266-relay@1.0.0
  Build timestamp: May 12 2019 @ 23:14:17
  35160 MEM-DELTA: -5192

  34928 MEM-DELTA: -232
  Secure API checkin...
  *TH: API connection failed.
  11456 MEM-DELTA: -23472
  THiNX/MQTT connected, reporting initial status.
  11344 MEM-DELTA: -112

   */
  THiNX::logging = true;

  thx = THiNX(apikey, owner_id);

  // You can override versioning with your own app identifier before
  // THiNX checks-in after the first thx.loop();
  thx.thinx_firmware_version = "cloud.thinx.esp8266-relay@1.0.0";
  thx.thinx_firmware_version_short = "1.0.0";

  Serial.println(thx.thinx_firmware_version);
  Serial.print("Build timestamp: ");  // helps debugging failing USB updates until version is changed intentionally
  Serial.print(__DATE__);
  Serial.print(" @ ");
  Serial.println(__TIME__);

  //
  // Callbacks
  //

  // Called after library gets connected and registered, optional.
  thx.setFinalizeCallback([] {
    Serial.println("THiNX/MQTT connected, reporting initial status.");
    report_status();
  });

  // Called after MQTT message arrives
  thx.setMQTTCallback([](byte* raw_message) {
    String message = String((const char*)raw_message);
    process_message(message);
  });

  debug_mem();
}

// logs free heap on change, helps debugging memory leaks
unsigned long freeHeap = ESP.getFreeHeap();
void debug_mem() {
  if (ESP.getFreeHeap() != freeHeap) {
    Serial.print(ESP.getFreeHeap()); Serial.print(" MEM-DELTA: ");
    uint32_t newHeap = ESP.getFreeHeap();
    if (freeHeap > newHeap) {
      // + f-n
      Serial.print("-");
      Serial.println(freeHeap-newHeap);

    } else {
      // + n-f
      Serial.print("+");
      Serial.println(newHeap-freeHeap);
    }
    // save last heap for next time...
    freeHeap = ESP.getFreeHeap();
  }
}

/* Loop must call the thx.loop() in order to pickup MQTT messages and advance the state machine. */
void loop(void) {
  thx.loop(); // THiNX/MQTT run-loop
  server.handleClient(); // HTTP server run-loop
  debug_mem();
}
