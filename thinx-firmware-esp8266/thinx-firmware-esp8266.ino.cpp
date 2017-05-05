# 1 "/var/folders/yp/2l8w2hpd08199zstwqnxnvs80000gn/T/tmpZQj1Sp"
#include <Arduino.h>
# 1 "/Users/sychram/Repositories/thinx-firmware-esp8266/thinx-firmware-esp8266/thinx-firmware-esp8266.ino"




#include "Arduino.h"

#define __DEBUG__ 


#define __USE_WIFI_MANAGER__ 

#include <stdio.h>
#include <Arduino.h>
#include "ArduinoJson/ArduinoJson.h"

#include "Thinx.h"
#include "FS.h"


#ifndef __USE_WIFI_MANAGER__
#include "Settings.h"
#else






#include "EAVManager/EAVManager.h"
#endif







#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>


const char* autoconf_ssid = "AP-THiNX";
const char* autoconf_pwd = "PASSWORD";


char thx_api_key[40];
char thx_udid[64] = {0};
EAVManagerParameter api_key_param("apikey", "API Key", thx_api_key, 40);


WiFiClient thx_wifi_client;
int status = WL_IDLE_STATUS;

PubSubClient thx_mqtt_client(thx_wifi_client);
int last_mqtt_reconnect;

bool shouldSaveConfig = false;
bool connected = false;
void setup();
void THiNX_initWithAPIKey(String api_key);
void esp_update(String url);
void thinx_parse(String payload);
String thinx_mqtt_channel();
String thinx_mac();
void checkin();
void senddata(String body);
void mqtt();
bool thinx_mqtt_reconnect();
void thinx_mqtt_callback(char* topic, byte* payload, unsigned int length);
void configModeCallback (EAVManager *myEAVManager);
void saveConfigCallback();
void connect();
bool restoreDeviceInfo();
void saveDeviceInfo();
String deviceInfo();
void loop();
#line 62 "/Users/sychram/Repositories/thinx-firmware-esp8266/thinx-firmware-esp8266/thinx-firmware-esp8266.ino"
void setup() {
  Serial.begin(115200);
  while (!Serial);

  THiNX_initWithAPIKey(thinx_api_key);
}




void THiNX_initWithAPIKey(String api_key) {

  if (api_key != "") {
    thinx_api_key = api_key;
    sprintf(thx_api_key, "%s", thinx_api_key.c_str());
  }

  sprintf(thx_udid, "%s", thinx_udid.c_str());

  restoreDeviceInfo();

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
  checkin();

#ifdef __DEBUG__



#endif
}



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
    Serial.println("[update] Update ok.");
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
      Serial.println("[update] Update ok.");
      break;
    }
  }
}



static const String thx_connected_response = "{ \"status\" : \"connected\" }";
static const String thx_disconnected_response = "{ \"status\" : \"disconnected\" }";



void thinx_parse(String payload) {






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


      String commit = registration["commit"];
      Serial.println(String("commit: ") + commit);


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

        esp_update(url);
#endif
      }
    } else {
      Serial.println(String("Unhandled status: ") + status);
    }
}
# 263 "/Users/sychram/Repositories/thinx-firmware-esp8266/thinx-firmware-esp8266/thinx-firmware-esp8266.ino"
String thinx_mqtt_channel() {
  return String("/thinx/device/") + thinx_mac();
}







String thinx_mac() {

  byte mac[] = {
    0xDE, 0xFA, 0x01, 0x70, 0x32, 0x42
  };

  WiFi.macAddress(mac);
  char macString[16] = {0};
  sprintf(macString, "5CCF7F%6X", ESP.getChipId());
  return String(macString);
}



void checkin() {

  Serial.println("*TH: Starting API checkin...");

  if(!connected) {
    Serial.println("*TH: Cannot checkin while not connected, exiting.");
    return;
  }


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



void senddata(String body) {

  char shorthost[256] = {0};
  sprintf(shorthost, "%s", thinx_cloud_url.c_str());


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
    thx_wifi_client.println("Accept: application/json");
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







void configModeCallback (EAVManager *myEAVManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myEAVManager->getConfigPortalSSID());
}


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







void connect() {
  #ifdef __USE_WIFI_MANAGER__
  EAVManager EAVManager;

  EAVManager.addParameter(&api_key_param);

  EAVManager.setAPCallback(configModeCallback);
  EAVManager.setTimeout(10000);
  EAVManager.autoConnect(autoconf_ssid,autoconf_pwd);
  #else
  status = WiFi.begin(ssid, pass);
  #endif


  while ( !connected ) {
    #ifdef __USE_WIFI_MANAGER__
    status = EAVManager.autoConnect(autoconf_ssid,autoconf_pwd);
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





bool restoreDeviceInfo() {

  Serial.println("Mounting SPIFFS for reading...");
  bool result = SPIFFS.begin();
  delay(10);
  Serial.println("SPIFFS mounted: " + result);

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
      if (strlen(saved_alias) > 1) {
        thinx_alias = String(saved_alias);
      }

      const char* saved_owner = config["owner"];
      if (strlen(saved_owner) > 5) {
        thinx_owner = String(saved_owner);
      }

      const char* saved_apikey = config["apikey"];
      if (strlen(saved_apikey) > 8) {
       thinx_api_key = String(saved_apikey);
       sprintf(thx_api_key, "%s", saved_apikey);
      }

      const char* saved_udid = config["udid"];
      Serial.print("Saved udid: "); Serial.println(saved_udid);
      if ((strlen(saved_udid) == 12) || (strlen(saved_udid) == 40)) {
       thinx_udid = String(saved_udid);
       sprintf(thx_udid, "%s", saved_udid);
     } else {
       thinx_udid = thinx_mac();
       sprintf(thx_udid, "%s", saved_udid);
     }
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





  #endif

  SPIFFS.end();
}


void saveDeviceInfo()
{
  Serial.println("Skipping saveDeviceInfo, depending on Thinx.h and in-memory so far...");

  return;

  Serial.setDebugOutput(true);

  Serial.println("Mounting SPIFFS for writing...");
  bool result = SPIFFS.begin();
  delay(10);
  Serial.println("SPIFFS mounted: " + result);

  Serial.println("*TH: Opening/creating config file...");



  File f = SPIFFS.open("/thinx.cfg", "w+");
  if (!f) {
    Serial.println("*TH: Cannot save configuration, formatting SPIFFS...");
    SPIFFS.format();
    Serial.println("*TH: Trying to save again...");
    f = SPIFFS.open("/thinx.cfg", "w+");
    if (f) {
      saveDeviceInfo();
    } else {
      Serial.println("*TH: Retry failed...");
    }
  } else {
    Serial.print("*TH: saving configuration: ");
    String config = deviceInfo();
    Serial.println(config);

    f.println(config.c_str());

    Serial.println("*TH: closing file crashes here...");
    delay(100);
    f.close();
  }
  Serial.println("*TH: saveDeviceInfo() completed.");
  SPIFFS.end();
}

String deviceInfo()
{
  StaticJsonBuffer<480> jsonBuffer;
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