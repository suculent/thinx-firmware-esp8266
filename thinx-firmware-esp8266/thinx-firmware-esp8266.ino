/* OTA enabled firmware for Wemos D1 (ESP 8266, Arduino) */

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

#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// #include <WiFiClient.h> // from ESP8266WiFi



// Default OTA login
const char* autoconf_ssid  = "THiNX_FIRMWARE_AP"; // SSID in AP mode
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

  delay(500);

  connect();

  Serial.println("*TH: Connected to WiFi...");

  checkin();

  Serial.println("*TH: Contacting MQTT...");

  mqtt();
  // test == our tenant name from THINX platform
  // Serial.println("[update] Trying direct update...");
  // esp_update("/bin/test/firmware.elf");
}

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

void thinx_mqtt_callback(char* topic, byte* payload, unsigned int length) {

  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';

  String s_topic = String(topic);
  String s_payload = String(c_payload);

  Serial.println("MQTT Callback:");
  Serial.println(topic);
  Serial.println(c_payload);

  if ( s_topic == thinx_mqtt_channel() ) {
    thinx_parse(c_payload);
  }
}

static const String thx_connected_response = "{ \"status\" : \"connected\" }";
static const String thx_disconnected_response = "{ \"status\" : \"disconnected\" }";

bool thinx_mqtt_reconnect() {

  Serial.print(thinx_mqtt_channel());
  Serial.println(" (re)connecting...");
  String channel = thinx_mqtt_channel();
  String mac = thinx_mac();

  if ( thx_mqtt_client.connect(mac.c_str(),channel.c_str(),0,false,thx_disconnected_response.c_str()) ) {
    Serial.println("Connected.");
    if (thx_mqtt_client.subscribe(channel.c_str())) {
      Serial.print(channel);
      Serial.println(" subscribed.");
    } else {
      Serial.println("Not subscribed.");
    }
    thx_mqtt_client.publish(channel.c_str(), thx_connected_response.c_str());
  } else {
    Serial.println("MQTT Not connected.");
  }
  return thx_mqtt_client.connected();
}

void thinx_parse(String payload) {

  StaticJsonBuffer<200> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(payload.c_str());

  // TODO: Unwrap JSON payload:

  if (root["registration"]) {

    /*
    - registration:
    - success: true,
    - status: "OK"
    */

    bool success = root["registration"]["success"];
    String status = root["registration"]["status"];

    // TODO: if status == OK
    // TODO: Extract alias override
    // TODO: Extract owner override (?) SEC?

    // TODO: if status == FIRMWARE_UPDATE



    // TODO: Check if this is update or what?



    String mac = root["registration"]["mac"];
    // TODO: must be this or ANY

    String commit = root["registration"]["commit"];
    // TODO: must not be this

    String version = root["registration"]["version"];

    String sha256 = root["registration"]["sha256"];
    // TODO: use this by modifying ESP8266httpUpdate esp_update(url, hash);

    Serial.println("Starting update...");

    bool forced = root["registration"]["forced"];

    if (forced == true) {
      String url = root["registration"]["url"];
      String update_url = thinx_cloud_url + url;
      Serial.println("Update URL:" + update_url);
      // TODO: must not contain HTTP, extend with http://thinx.cloud/" // could use node.js as a secure provider instead of Apache!
      esp_update(update_url);
    }
  }
}

/*
* Designated MQTT channel for each device. In case devices know their channels, they can talk together.
* Update must be skipped unless forced and matching MAC, even though a lot should be validated to prevent overwrite.
*/

String thinx_mqtt_channel() {
  return String("/thinx/device/") + thinx_mac();
}

/*
* May return hash only in future.
*/

String thinx_mac() {

  byte mac[] = {
    0xDE, 0xFA, 0x01, 0x70, 0x32, 0x42
  }; // 0x5C, 0xCF, 0x7F, 0xF0, 0x9C, 0x39

  WiFi.macAddress(mac);
  char macString[16] = {0};
  sprintf(macString, "THiNX:%06X", ESP.getChipId());
  return String(macString);
}

void checkin() {

  Serial.println("[setup] Contacting API...");

  if(!connected) {
    Serial.println("Cannot checkin while not connected, exiting...");
    return;
  }

  // Default MAC address for AP controller
  byte mac[] = {
    0xDE, 0xFA, 0xDE, 0xFA, 0xDE, 0xFA
  };

  Serial.println("Building JSON Response...");

  StaticJsonBuffer<2000> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["mac"] = thinx_mac();
  root["firmware"] = thinx_firmware_version;
  root["hash"] = thinx_commit_id;
  root["owner"] = thinx_owner;
  root["alias"] = thinx_alias;

  Serial.println("Wrapping JSON Response...");

  StaticJsonBuffer<2048> wrapperBuffer;
  JsonObject& wrapper = wrapperBuffer.createObject();
  wrapper["registration"] = root;
  Serial.print("JSON: ");
  wrapper.printTo(Serial);

  senddata(root);
}


void senddata(JsonObject& json) {

  char host[256] = {0};
  sprintf(host, "http://%s:7442", thinx_cloud_url);

  thx_wifi_client.print("POST /device/register"); thx_wifi_client.println(" HTTP/1.1");
  thx_wifi_client.print("User-Agent: THiNX-Client");
  thx_wifi_client.print("Content-Type: application/json");
  int length = json.measureLength();
  thx_wifi_client.print("Content-Length:"); thx_wifi_client.println(length);
  thx_wifi_client.println(); // End of headers

  // POST message body
  //json.printTo(client); // very slow ??

  String out;
  json.printTo(out);
  Serial.print("POST BODY: ");
  Serial.println(out);
  Serial.print("Sending body...");
  thx_wifi_client.println(out);

  long interval = 5000;
  unsigned long currentMillis = millis(), previousMillis = millis();

  while(!thx_wifi_client.available()) {
    if( (currentMillis - previousMillis) > interval ){
      Serial.println("Client not available (timeout)!");
      thx_wifi_client.stop();
      return;
    }
    currentMillis = millis();
  }


  char response[2048];
  int index = 0;
  while (thx_wifi_client.connected()) {
    if ( thx_wifi_client.available() ) {
      char str = thx_wifi_client.read();
      response[index] = str;
      Serial.println(str);
      index++;
    }
  }
  response[index] = '\0';

  Serial.println(String(response));

  thinx_parse(response);
}

//
// MQTT Connection
//

void mqtt() {
  thx_mqtt_client.setServer(thinx_mqtt_url.c_str(), thinx_mqtt_port);
  thx_mqtt_client.setCallback(thinx_mqtt_callback);
  thinx_mqtt_reconnect();
  last_mqtt_reconnect = 0;
}

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

//
// WiFi Connection
//

void connect() {
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
    Serial.print("Waiting for WiFiManager autoconnect... ");
    status = wifiManager.autoConnect(autoconf_ssid,autoconf_pwd);
    Serial.println("*TH: Connected... ");
    if (status == true) {
      Serial.println("*TH: Status: true... ");
      connected = true;
      //break;
    } else {
      Serial.print("*TH: Status: false... ");
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

// TODO: Add client receive data to fetch update info on registration. Optional for forced updates, not required at the moment; exit by calling`thinx_parse(c_payload);`

void loop() {

}
