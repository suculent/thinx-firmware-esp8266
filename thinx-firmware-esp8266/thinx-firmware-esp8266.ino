/* OTA enabled firmware for Wemos D1 (ESP 8266, Arduino) */

// version 1.3.28

#include "Arduino.h"

#define __DEBUG__
// #define __DEBUG_JSON__

#define __USE_WIFI_MANAGER__

#include <stdio.h>
#include <Arduino.h>
#include "ArduinoJson/ArduinoJson.h"

#include "Thinx.h"
#include "FS.h"

// Inject SSID and Password from 'Settings.h' for testing where we do not use WiFiManager
#ifndef __USE_WIFI_MANAGER__
#include "Settings.h"
#else
// Custom clone of WiFiManager (we shall revert back to OpenSource if this won't be needed)
// Purpose: SSID/password injection in AP mode
// Solution: re-implement from UDP in mobile application
//
// Changes so far: `int connectWifi()` moved to public section in header
// - buildable, but requires UDP end-to-end)
#include "WiFiManager/WiFiManager.h"
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

// Requires API v126+
char thx_api_key[40];
char thx_udid[64];
WiFiManagerParameter api_key_param("apikey", "API Key", thx_api_key, 40);

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

  THiNX_initWithAPIKey(thinx_api_key); // init with unique API key you obtain from web and paste to in Thinx.h
}

/* Should be moved to library constructor */

// Designated initialized
void THiNX_initWithAPIKey(String api_key) {

  if (api_key != "") {
    thinx_api_key = api_key;
    sprintf(thx_api_key, "%s", thinx_api_key.c_str()); // 40 max
  }

  Serial.println("Mounting SPIFFS...");
  bool result = SPIFFS.begin();
  delay(10);
  Serial.println("SPIFFS mounted: " + result);

  restoreDeviceInfo();

  sprintf(thx_udid, "%s", thinx_udid.c_str());

  connect();

#ifdef __DEBUG__
  if (connected) {
    Serial.println("*TH: Connected to WiFi...");
  } else {
    Serial.println("*TH: Connection to WiFi failed...");
  }
#endif

  delay(500);
  mqtt();

  delay(5000);
  checkin(); // crashes so far in HTTP POST

#ifdef __DEBUG__
  // test == our tenant name from THINX platform
  // Serial.println("[update] Trying direct update...");
  // esp_update("/bin/test/firmware.elf");
#endif
}

/* Should be moved to private library method */

void esp_update(String url) {

#ifdef __DEBUG__
  Serial.println("[update] Starting update...");
#endif

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
    Serial.println("[update] TODO: Rewrite to secure binary provider on the API side!");
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

  // TODO: Should parse response only for this device_id (which must be internal and not a mac)
  // TODO: store device_id, alias and owner using SPIFFS in thinx.json
  // TODO: status can be OK or FIRMWARE_UPDATE; ignore rest
  // {"registration":{"success":true,"status":"OK","alias":"","owner":"","device_id":"5CCF7FF09C39"}}

#ifdef __DEBUG__
  Serial.print("Parsing response: ");
  Serial.println(payload);
#endif

  int startIndex = payload.indexOf("\n{");
  String body = payload.substring(startIndex + 1);

  StaticJsonBuffer<4096> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(body.c_str());

  if ( !root.success() ) {
#ifdef __DEBUG__
    Serial.println("Failed parsing root node.");
#endif
    return;
  }

  JsonObject& registration = root["registration"];

  if ( !registration.success() ) {
#ifdef __DEBUG__
    Serial.println("Failed parsing registration node.");
#endif
    return;
  }

    bool success = registration["success"];
    String status = registration["status"];

    #ifdef __DEBUG__
      Serial.print("success: ");
      Serial.println(success);

      Serial.print("status: ");
      Serial.println(status);
    #endif

    if (status == "OK") {

      String alias = registration["alias"];
      if ( alias.length() > 0 ) {
        Serial.println(String("assigning alias: ") + alias);
        thinx_alias = alias;
      }

      String owner = registration["owner"];
      if ( owner.length() > 0 ) {
        Serial.println(String("assigning owner: ") + owner);
        thinx_owner = owner;
      }

      String udid = registration["device_id"];
      if ( udid.length() > 0 ) {
        Serial.println(String("assigning device_id: ") + udid);
        thinx_udid = udid;
      }

      saveDeviceInfo();
      return;

    } else if (status == "FIRMWARE_UPDATE") {

      String mac = registration["mac"];
      Serial.println(String("mac: ") + mac);
      // TODO: must be this or ANY

      String commit = registration["commit"];
      Serial.println(String("commit: ") + commit);

      // should not be this
      if (commit == thinx_commit_id) {
        Serial.println("*TH: Warning: new firmware has same commit_id as current.");
      }

      String version = registration["version"];
      Serial.println(String("version: ") + version);

      Serial.println("Starting update...");

      String url = registration["url"];
      if (url) {
#ifdef __DEBUG__
        Serial.println("*TH: SKIPPING force update with URL:" + url);
#else
        // TODO: must not contain HTTP, extend with http://thinx.cloud/" // could use node.js as a secure provider instead of Apache!
        esp_update(url);
#endif
      }
    } else {
      Serial.println(String("Unhandled status: ") + status);
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

  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["mac"] = thinx_mac();
  root["firmware"] = String(thinx_firmware_version);
  root["version"] = String(thinx_firmware_version_short);
  root["hash"] = String(thinx_commit_id);
  root["owner"] = String(thinx_owner);
  root["alias"] = String(thinx_alias);
  root["device_id"] = String(thx_udid);

  StaticJsonBuffer<512> wrapperBuffer;
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

  char shorthost[256] = {0};
  sprintf(shorthost, "%s", thinx_cloud_url.c_str());

  // Response payload placeholder
  String payload = "{}";

#ifdef __DEBUG__
  Serial.print("*TH: Sending to ");
  Serial.println(shorthost);
  Serial.println(body);
#endif

  Serial.print("*TH: thx_api_key API KEY "); Serial.println(thx_api_key);

  if (thx_wifi_client.connect(shorthost, 7442)) {
    thx_wifi_client.println("POST /device/register HTTP/1.1");
    thx_wifi_client.println("Host: thinx.cloud");
    thx_wifi_client.print("Authentication: "); thx_wifi_client.println(thx_api_key);
    thx_wifi_client.println("Accept: application/json"); // application/json
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
        Serial.println("Response Timeout. TODO: Should retry later.");
        thx_wifi_client.stop();
        return;
      }
      currentMillis = millis();
    }

    while ( thx_wifi_client.connected() ) {
      if ( thx_wifi_client.available() ) {
        char str = thx_wifi_client.read();
        Serial.print(str);
        payload = payload + String(str);
      }
    }

    Serial.println();

    thx_wifi_client.stop();

    thinx_parse(payload);

  } else {
    Serial.println("*TH: API connection failed.");
    return;
  }
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
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

// `api_key_param` should have its value set when this gets called
void saveConfigCallback() {
  Serial.println("Save config callback:");
  strcpy(thx_api_key, api_key_param.getValue());
  if (String(thx_api_key).length() > 0) {
    thinx_api_key = String(thx_api_key);
    Serial.print("Saving thinx_api_key: ");
    Serial.println(thinx_api_key);
    saveDeviceInfo();
  }
}

/* Private library method */

//
// WiFi Connection
//

void connect() { // should return status bool
  #ifdef __USE_WIFI_MANAGER__
  WiFiManager WiFiManager;

  WiFiManager.addParameter(&api_key_param);

  WiFiManager.setAPCallback(configModeCallback);
  WiFiManager.setTimeout(10000);
  WiFiManager.autoConnect(autoconf_ssid,autoconf_pwd);
  #else
  status = WiFi.begin(ssid, pass);
  #endif

  // attempt to connect to Wifi network:
  while ( !connected ) {
    #ifdef __USE_WIFI_MANAGER__
    status = WiFiManager.autoConnect(autoconf_ssid,autoconf_pwd);
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

//
// PERSISTENCE
//

bool restoreDeviceInfo() {

  File f = SPIFFS.open("/thinx.cfg", "r");
  if (!f) {
      Serial.println("*TH: Cannot restore configuration.");
      return false;
  } else {
    String data = f.readStringUntil('\n');
    StaticJsonBuffer<256> jsonBuffer;
    JsonObject& config = jsonBuffer.parseObject(data.c_str());
    if (!config.success()) {
      Serial.println("*TH: parseObject() failed");
    } else {

      const char* saved_alias = config["alias"];
      if (strlen(saved_alias) > 0) {
        thinx_alias = String(saved_alias);
      }

      const char* saved_owner = config["owner"];
      if (strlen(saved_owner) > 0) {
        thinx_owner = String(saved_owner);
      }

      const char* saved_apikey = config["apikey"];
      if (strlen(saved_apikey) > 0) {
       thinx_api_key = String(saved_apikey);
       sprintf(thx_api_key, "%s", saved_apikey); // 40 max
      }

      const char* saved_udid= config["udid"];
      if (strlen(saved_udid) > 0) {
       thinx_udid = String(saved_udid);
       sprintf(thx_udid, "%s", saved_udid); // 40 max
      }

      // TODO: device_id
    }
  }

  #ifdef __DEBUG__
    Serial.println("*TH: Restored configuration:");

    Serial.print("     Alias: ");
    Serial.println(thinx_alias);
    Serial.print("     Owner: ");
    Serial.println(thinx_owner);
    Serial.print("     API Key: ");
    Serial.println(thx_api_key);
    Serial.print("     UDID: ");
    Serial.println(thx_udid);

    Serial.print("     Firmware: ");
    Serial.println(thinx_firmware_version);

    //Serial.print("*TH: DEBUG Build-time Owner: ");
    //Serial.println(thinx_owner);
    //Serial.print("*TH: DEBUG Build-time Device Alias: ");
    //Serial.println(thinx_alias);
  #endif
}

/* Stores mutable device data (alias, owner) retrieved from API */
void saveDeviceInfo()
{
  Serial.println("*TH: Opening/creating config file...");
  File f = SPIFFS.open("/thinx.cfg", "w");
  if (!f) {
    Serial.println("*TH: Cannot save configuration, formatting SPIFFS...");
    SPIFFS.format();
    Serial.println("*TH: Trying to save again..."); // TODO: visual feedback
    saveDeviceInfo();
  } else {
    Serial.print("*TH: saving configuration:");
    String config = deviceInfo();
    Serial.println(config);
    Serial.println("*TH: saveConfiguration completed. (warning: CRASH FOLLOWS) >>>");
    f.println(config);
    Serial.println("*TH: closing file...");
    f.close();
    delay(100); // let the file close
  }
  Serial.println("*TH: saveDeviceInfo() completed.");
}

String deviceInfo()
{
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["alias"] = thinx_alias;
  root["owner"] = thinx_owner;
  root["apikey"] = thx_api_key;
  root["udid"] = thinx_udid;

  String jsonString;
  root.printTo(jsonString);

  return jsonString;
}

void loop()
{

  delay(1000);
  Serial.println(".");
}
