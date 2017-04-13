/* OTA enabled firmware for Wemos D1 (ESP 8266, Arduino) */

#include <stdio.h>
#include <Arduino.h>

#include "../Thinx.h"

String app_version = "thinx-esp8266-vanilla-0.0.1-alpha:2017/04/13"; // requires dependency injection and a build-chain

// IN PROGRESS: Send a registration post request with current MAC, Firmware descriptor, commit ID; sha and version if known (with all other useful params like expected device owner).

// TODO: Add UDP AT&U= responder like in EAV? Considered unsafe. Device will notify available update and download/install it on its own (possibly throught THiNX Security Gateway (THiNX )
// IN PROGRESS: Add MQTT client (target IP defined using Thinx.h) and forced firmware update responder (will update on force or save in-memory state from new or retained mqtt notification)
// TODO: Add UDP responder AT&U only to update to next available firmware (from save in-memory state)
// TODO: Add cheese.
// TODO: 


#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>

#include "../Thinx.h" 


// Requires connectivity, that should be served through WiFiManager
char ssid[] = "<ssid>";     //  your network SSID (name)
char pass[] = "<password>";  // your network password

// Default OTA login
const char* autoconf_ssid  = "THINX_ESP8266_OTA"; // SSID in AP mode
const char* autoconf_pwd   = "PASSWORD"; // fallback to default password, however this should be generated uniquely and logged to console for substantially better security

WiFiClient espClient;
PubSubClient client(espClient);

int status = WL_IDLE_STATUS;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  WiFiManager wifiManager;
  wifiManager.autoConnect(autoconf_ssid,autoconf_pwd);
  //setup_ota();

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while(true);
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }

  delay(500);

  Serial.println("[update] WiFi connected...");  

  client.setServer(thinx_mqtt_url, thinx_mqtt_port);
  client.setCallback(thinx_mqtt_callback);
  reconnect();
  lastReconnectAttempt = 0;
  
  checkin();

  // test == our tenant name from THINX platform  
  // Serial.println("[update] Trying direct update...");  
  // esp_update("/bin/test/firmware.elf");
}

void esp_update(char* url) {

  Serial.println("[update] Starting update...");  
  
  t_httpUpdate_return ret = ESPhttpUpdate.update("images.thinx.cloud", 80, url);

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
}

void thinx_parse(payload) {
  char json[] = payload.c_str();

      StaticJsonBuffer<200> jsonBuffer;
      
      JsonObject& root = jsonBuffer.parseObject(json);

      // TODO: Unwrap JSON payload:

      if (root["registration"] {

         /*
      {
        "registration" : {
            "success" : true,
            "status" : "OK"
        }
      }
       */

      // TODO: if status == OK
      // TODO: Extract alias override
      // TODO: Extract owner override (?) SEC?

      // TODO: if status == FIRMWARE_UPDATE
      

  /*
    {
        "registration" : {
            "success" : true,
            "status" : "FIRMWARE_UPDATE",            
            "url" : "/bin/test/firmware.elf",
            "mac" : "5C:CF:7F:EE:90:E0;ANY",
            "commit" : "18ee75e3a56c07a9eff08f75df69ef96f919653f",
            "version" : "0.1",
            "sha256" : "6bf6bd7fc983af6c900d8fe162acc3ba585c446ae0188e52802004631d854c60"
        }
    }
    */

      // TODO: Check if this is update or what?
      
      bool success = root["registration"]["success"];
      String status = root["registration"]["status"];      
      
      String mac = root["registration"]["mac"];
      // TODO: must be this or ANY
      
      String commit = root["registration"]["commit"];
      // TODO: must not be this

      String version = root["registration"]["version"];
      
      String sha256 = root["registration"]["sha256"];
      // TODO: use this by modifying ESP8266httpUpdate esp_update(url, hash);
            
      Serial.println("Starting update with "+url);

      bool forced = root["registration"]["forced"];

      if (forced == true) {
        String url = root["registration"]["url"];
        // TODO: must not contain HTTP, extend with http://images.thinx.cloud/" // could use node.js as a secure provider instead of Apache!
        esp_update(url);
      } 
      }

String thinx_firmware_version() {
  return app_version;
}

String thinx_mqtt_channel() {
  return "/thinx/device/" + thinx_mac();
}

String thinx_mac() {
  WiFi.macAddress(mac);
  char macString[16] = {0};
  sprintf(macString, "THiNX:%06X", ESP.getChipId());
  return String(macString);
}

void checkin() {
  // TODO: if((WiFiMulti.run() == WL_CONNECTED)) {

  // Default MAC address for AP controller
  byte mac[] = {
    0xDE, 0xFA, 0xDE, 0xFA, 0xDE, 0xFA
  };

  StaticJsonBuffer<200> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

    JsonObject& registration = jsonBuffer.createObject();
    registration["mac"] = thinx_mac();
    registration["firmware"] = thinx_firmware_version();
    registration["hash"] = thinx_commit_id; 
    registration["owner"] = thinx_owner;
    registration["alias"] = thinx_alias;
  
  root["registration"] = registration;

  root.printTo(Serial);

  senddata(root);
}


void senddata(JsonObject& json)
{
  WiFiClient client = espClient;
  
  const char* host="http://thinx.cloud:7442";
  
  client.print("POST /device/register"); client.println(" HTTP/1.1");
  client.print("User-Agent: THiNX-Client");
  client.print("Content-Type: application/json")
  int length = json.measureLength();
  client.print("Content-Length:"); client.println(length);  
  client.println(); // End of headers
  // POST message body
  //json.printTo(client); // very slow ??
  String out;
  json.printTo(out);
  client.println(out);
}

// TODO: Add client receive data to fetch update info on registration? Or this is a nonsense and we'll get it only over MQTT and update if forced?
// then use thinx_parse(c_payload);    
