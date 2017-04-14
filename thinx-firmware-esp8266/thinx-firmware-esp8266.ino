/* OTA enabled firmware for Wemos D1 (ESP 8266, Arduino) */

#define __DEBUG__
// #define __DEBUG_JSON__

// #define __USE_HTTPCLIENT__
#define __USE_ESP8266WIFI__

#define __USE_WIFI_MANAGER__

#include <stdio.h>
#include <Arduino.h>
#include <ArduinoJson.h>

#include "Thinx.h"

// Inject SSID and Password from 'Settings.h' where we do not use WiFiManager
#ifndef __USE_WIFI_MANAGER__
#include "Settings.h"
#else
#include <WiFiManager.h>
#endif

// WORK IN PROGRESS: Send a registration post request with current MAC, Firmware descriptor, commit ID; sha and version if known (with all other useful params like expected device owner).

// TODO: Add UDP AT&U= responder like in EAV? Considered unsafe. Device will notify available update and download/install it on its own (possibly throught THiNX Security Gateway (THiNX )
// IN PROGRESS: Add MQTT client (target IP defined using Thinx.h) and forced firmware update responder (will update on force or save in-memory state from new or retained mqtt notification)
// TODO: Add UDP responder AT&U only to update to next available firmware (from save in-memory state)

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>

// Default OTA login
const char* autoconf_ssid  = "AP-THiNX"; // SSID in AP mode
const char* autoconf_pwd   = "PASSWORD"; // fallback to default password, however this should be generated uniquely as it is logged to console

// WiFiClient is required by PubSubClient and HTTP POST
WiFiClient thx_wifi_client;
int status = WL_IDLE_STATUS;

PubSubClient thx_mqtt_client(thx_wifi_client);
int last_mqtt_reconnect;

bool shouldSaveConfig = false;
bool connected = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  thinx_init();
}

/* Should be moved to library constructor */

void thinx_init() {

  connect();

  if (connected) {
    Serial.println("*TH: Connected to WiFi...");
  } else {
    Serial.println("*TH: Connection to WiFi failed...");
  }

  delay(500);
  mqtt();

  delay(5000);
  checkin(); // crashes so far in HTTP POST

  // test == our tenant name from THINX platform
  // Serial.println("[update] Trying direct update...");
  // esp_update("/bin/test/firmware.elf");
}

/* Should be moved to private library method */

void esp_update(String url) {

  Serial.println("[update] Starting update...");

  t_httpUpdate_return ret = ESPhttpUpdate.update("thinx.cloud", 80, url.c_str());

  switch(ret) {
    case HTTP_UPDATE_FAILED:
    Serial.println("[update] Update failed.");
    break;
    case HTTP_UPDATE_NO_UPDATES:
    Serial.println("[update] Update no Update.");
    break;
    case HTTP_UPDATE_OK:
    Serial.println("[update] Update ok."); // may not called we reboot the ESP
    delay(1000);
    break;
  }

  if (ret != HTTP_UPDATE_OK) {
    Serial.println("[update] WiFi connected, trying advanced update...");

    ret = ESPhttpUpdate.update("images.thinx.cloud", 80, "ota.php", "5ccf7fee90e0");

    switch(ret) {
      case HTTP_UPDATE_FAILED:
      Serial.println("[update] Update failed.");
      break;
      case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[update] Update no Update.");
      break;
      case HTTP_UPDATE_OK:
      Serial.println("[update] Update ok."); // may not called we reboot the ESP
      break;
    }
  }
}

/* Private library definitions */

static const String thx_connected_response = "{ \"status\" : \"connected\" }";
static const String thx_disconnected_response = "{ \"status\" : \"disconnected\" }";

/* Private library method */

void thinx_parse(String payload) {

  // {"registration":{"success":true,"status":"OK","alias":"","owner":"","device_id":"00:00:00:00:00:00"}}I

  Serial.print("Parsing response: ");
  Serial.println(payload);

  StaticJsonBuffer<4096> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(payload.c_str());

  if ( !root.success() ) {
    Serial.println("Failed parsing root node.");
    return;
  }

  JsonObject& registration = root["registration"];

    if ( !registration.success() ) {
      Serial.println("Failed parsing registration node.");
      // return;
    }

    /*
    - registration:
    - success: true,
    - status: "OK"
    */

    bool success = registration["success"];
    String status = registration["status"];

    Serial.print("success: ");
    Serial.println(success);

    Serial.print("status: ");
    Serial.println(status);

    // TODO: if status == OK
    // TODO: Extract alias override
    // TODO: Extract owner override (?) SEC?

    // TODO: if status == FIRMWARE_UPDATE



    // TODO: Check if this is update or what?



    String mac = registration["mac"];
    // TODO: must be this or ANY

    String commit = registration["commit"];
    // TODO: must not be this

    String version = registration["version"];

    // LX6 cannot calculate hash in reasonable time
    // String sha256 = registration["sha256"];
    // TODO: use this by modifying ESP8266httpUpdate esp_update(url, hash);

    Serial.println("Starting update...");

    // bool forced = registration["forced"]; not supported yet
    bool forced = true; // for testing only to see URL unwrapping...

    if (forced == true) {
      String url = registration["url"];
      Serial.println("*TH: NOT Forcing update with URL:" + url);
      // TODO: must not contain HTTP, extend with http://thinx.cloud/" // could use node.js as a secure provider instead of Apache!
      //esp_update(url);
    }
}

/* Private library method */

/*
* Designated MQTT channel for each device. In case devices know their channels, they can talk together.
* Update must be skipped unless forced and matching MAC, even though a lot should be validated to prevent overwrite.
*/

String thinx_mqtt_channel() {
  return String("/thinx/device/") + thinx_mac();
}

/* Private library method */

/*
* May return hash only in future.
*/

String thinx_mac() {

  byte mac[] = {
    0xDE, 0xFA, 0x01, 0x70, 0x32, 0x42
  }; // 0x5C, 0xCF, 0x7F, 0xF0, 0x9C, 0x39

  WiFi.macAddress(mac);
  char macString[16] = {0};
  sprintf(macString, "5CCF7F%6X", ESP.getChipId());
  return String(macString);
}

/* Private library method */

void checkin() {

  Serial.println("*TH: Starting API checkin...");

  if(!connected) {
    Serial.println("*TH: Cannot checkin while not connected, exiting.");
    return;
  }

  // Default MAC address for AP controller
  byte mac[] = {
    0xDE, 0xFA, 0xDE, 0xFA, 0xDE, 0xFA
  };

  Serial.println("*TH: Building JSON...");

  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["mac"] = thinx_mac();
  root["firmware"] = thinx_firmware_version;
  root["hash"] = thinx_commit_id;
  root["owner"] = thinx_owner;
  root["alias"] = thinx_alias;

  StaticJsonBuffer<256> wrapperBuffer;
  JsonObject& wrapper = wrapperBuffer.createObject();
  wrapper["registration"] = root;
#ifdef __DEBUG_JSON__
  wrapper.printTo(Serial);
  Serial.println();
#endif

  String body;
  wrapper.printTo(body);

  senddata(body);
}

/* Private library method */

void senddata(String body) {

  // hostname with http://
  char host[256] = {0};
  sprintf(host, "http://%s", thinx_cloud_url.c_str());
  char shorthost[256] = {0};
  sprintf(shorthost, "%s", thinx_cloud_url.c_str());

  // whole URL string
  String url = String("http://") + thinx_cloud_url + String(":") + String(thinx_api_port) + String("/device/register");

  // Response payload placeholder
  String payload = "{}";

#ifdef __DEBUG__
  Serial.print("*TH: Sending to ");
  Serial.println(host);
  Serial.println(url);
  Serial.println(body);
  Serial.println(ESP.getFreeHeap());
#endif

// equivalent to WiFiClient
#ifdef __USE_ESP8266WIFI__
  if (thx_wifi_client.connect(shorthost, 7442)) {
    thx_wifi_client.println("POST /device/register HTTP/1.1");
    thx_wifi_client.println("Host: thinx.cloud");
    thx_wifi_client.println("Accept: */*"); // application/json
    thx_wifi_client.println("Origin: device");
    thx_wifi_client.println("Content-Type: application/json");
    thx_wifi_client.println("User-Agent: THiNX-Client");
    thx_wifi_client.print("Content-Length: ");
    thx_wifi_client.println(body.length());
    thx_wifi_client.println();
    Serial.println("Headers set...");
    thx_wifi_client.println(body);
    Serial.println("Body sent...");

    long interval = 2000;
    unsigned long currentMillis = millis(), previousMillis = millis();

    while(!thx_wifi_client.available()){
      if( (currentMillis - previousMillis) > interval ){
        Serial.println("Timeout");
        thx_wifi_client.stop();
        return;
      }
      currentMillis = millis();
    }

    while (thx_wifi_client.connected()) {
      if ( thx_wifi_client.available() ) {
        char str=thx_wifi_client.read();
        Serial.println(str);
        payload = payload + String(str);
      }
    }

    thx_wifi_client.close();

  } else {
    Serial.println("*TH: API connection failed.");
    return;
  }
#endif
// __USE_ESP8266WIFI__

// Using ESP8266HTTPClient crashes as well as anything else
#ifdef __USE_HTTPCLIENT__
  HTTPClient http;
  http.setTimeout(5000);
  Serial.println(ESP.getFreeHeap());

  Serial.println("HTTP BEGIN");
  //http.begin(url);
  http.begin(host, thinx_api_port, "/device/register");

  http.addHeader("Accept", "*/*"); // application/json
  http.addHeader("Origin", "device");
  http.addHeader("Content-Type", "application/json");
  http.setUserAgent("THiNX-Client");
  //http.setAuthorization("thinxdevice", "guest")

   Serial.println(ESP.getFreeHeap());

   Serial.print("POST:");
  int httpCode = http.POST(body);

  Serial.print("POST Completed");
  Serial.println(ESP.getFreeHeap());

  Serial.print("HTTP PAYLOAD");
  payload = http.getString();
  Serial.println(ESP.getFreeHeap());
  Serial.print("HTTPCode: ");
  Serial.println(httpCode);
  Serial.print("Response: ");
  Serial.println(payload);

  http.end();

  Serial.print("Parsing payload...");
  thinx_parse(payload);
#endif

}

/* Private library method */

//
// MQTT Connection
//

void mqtt() {
  Serial.print("*TH: Contacting MQTT server ");
  Serial.print(thinx_mqtt_url);
  Serial.print(" on port ");
  Serial.println(thinx_mqtt_port);
  thx_mqtt_client.setServer(thinx_mqtt_url.c_str(), thinx_mqtt_port);
  thx_mqtt_client.setCallback(thinx_mqtt_callback);
  thinx_mqtt_reconnect();
  last_mqtt_reconnect = 0;
}

/* Private library method */

bool thinx_mqtt_reconnect() {

  String channel = thinx_mqtt_channel();
  Serial.println("*TH: Connecting to MQTT...");

  String mac = thinx_mac();
  if ( thx_mqtt_client.connect(mac.c_str(),channel.c_str(),0,false,thx_disconnected_response.c_str()) ) {
    Serial.println("*TH: MQTT Connected.");
    if (thx_mqtt_client.subscribe(channel.c_str())) {
      Serial.print("*TH: ");
      Serial.print(channel);
      Serial.println(" successfully subscribed.");
    } else {
      Serial.println("*TH: Not subscribed.");
    }
    thx_mqtt_client.publish(channel.c_str(), thx_connected_response.c_str());
  } else {
    Serial.println("*TH: MQTT Not connected.");
  }
  return thx_mqtt_client.connected();
}

/* Private library method */

void thinx_mqtt_callback(char* topic, byte* payload, unsigned int length) {

  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';

  String s_topic = String(topic);
  String s_payload = String(c_payload);

  Serial.println("*TH: MQTT Callback:");
  Serial.println(topic);
  Serial.println(c_payload);

  if ( s_topic == thinx_mqtt_channel() ) {
    thinx_parse(c_payload);
  }
}

/* Optional methods */

//
// WiFiManager Setup Callbacks
//

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //mqtt_server = custom_mqtt_server.getValue();
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

/* Private library method */

//
// WiFi Connection
//

void connect() { // should return status bool
  #ifdef __USE_WIFI_MANAGER__
  WiFiManager wifiManager;

  // Add custom parameter when required:
  // id/name, placeholder/prompt, default, length
  //WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  //wifiManager.addParameter(&custom_mqtt_server);

  //wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(10000);
  wifiManager.autoConnect(autoconf_ssid,autoconf_pwd);
  #else
  status = WiFi.begin(ssid, pass);
  #endif

  // attempt to connect to Wifi network:
  while ( !connected ) {
    #ifdef __USE_WIFI_MANAGER__
    status = wifiManager.autoConnect(autoconf_ssid,autoconf_pwd);
    if (status == true) {
      connected = true;
      return;
    } else {
      Serial.print("*TH: Connection Status: false!");
      delay(3000);
      connected = false;
    }
    #else
    Serial.print("*TH: Connecting to SSID: ");
    Serial.print(ssid);
    Serial.print("*TH: Waiting for WiFi. Status: ");
    Serial.println(status);
    delay(3000);
    status = WiFi.begin(ssid, pass);
    if (status == WL_CONNECTED) {
      connected = true;
    }
    #endif
  }
}

// TODO: Add client receive data to fetch update info on registration.
// Optional for forced updates, not required at the moment;
// should exit by calling`thinx_parse(c_payload);`

void loop() {
  delay(5000); // supposed to help processing currently not-incoming MQTT callbacks
  if (thx_mqtt_client.connected() == false) {
    thinx_mqtt_reconnect();
  }
}
