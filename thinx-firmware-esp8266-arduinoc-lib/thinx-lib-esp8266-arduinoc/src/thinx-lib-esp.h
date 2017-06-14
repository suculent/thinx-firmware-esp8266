#ifndef THiNX_H
#define THiNX_H

#include <Arduino.h>
#define __DEBUG__
#define __DEBUG_JSON__

#define __DEBUG_WIFI__ /* use as fallback when device gets stucked with incorrect WiFi configuration */

#define __USE_WIFI_MANAGER__

#include <stdio.h>
#include <Arduino.h>
#include "ArduinoJson/ArduinoJson.h"

#include "FS.h"

// Inject SSID and Password from 'Settings.h' for testing where we do not use EAVManager
#ifndef __USE_WIFI_MANAGER__
#include "Settings.h"
#else
// Custom clone of EAVManager (we shall revert back to OpenSource if this won't be needed)
// Purpose: SSID/password injection in AP mode
// Solution: re-implement from UDP in mobile application
//
// Changes so far: `int connectWifi()` moved to public section in header
// - buildable, but requires UDP end-to-end)
#include "EAVManager/EAVManager.h"
#include <EAVManager.h>
#endif

// Using better than Arduino-bundled version of MQTT https://github.com/Imroy/pubsubclient
#include "PubSubClient/PubSubClient.h" // Local checkout
//#include <PubSubClient.h> // Arduino Library

// WORK IN PROGRESS: Send a registration post request with current MAC, Firmware descriptor, commit ID; sha and version if known (with all other useful params like expected device owner).

// TODO: Add UDP AT&U= responder like in EAV? Considered unsafe. Device will notify available update and download/install it on its own (possibly throught THiNX Security Gateway (THiNX )
// IN PROGRESS: Add MQTT client (target IP defined using Thinx.h) and forced firmware update responder (will update on force or save in-memory state from new or retained mqtt notification)
// TODO: Add UDP responder AT&U only to update to next available firmware (from save in-memory state)

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

WiFiClient thx_wifi_client;

char thx_api_key[64];
char thx_udid[64];

class THiNX {
  public:
    THiNX(String);
    THiNX();

    String thinx_alias;
    String thinx_owner;
    String thinx_api_key;

    String thinx_udid;
    String thinx_commit_id;
    String thinx_firmware_version;
    String thinx_firmware_version_short;
    String thinx_cloud_url;
    String thinx_mqtt_url;
    int thinx_mqtt_port;

    void initWithAPIKey(String);

    EAVManager manager;
    IPAddress mqtt_server;

    friend class EAVManager;

    //EAVManagerParameter api_key_param(); //(const char*, const char*, const char*, int, const char*);
    //friend class PubSubClient;



    private:



      const char* autoconf_ssid; // SSID in AP mode
      const char* autoconf_pwd; // fallback to default password, however this should be generated uniquely as it is logged to console

      // Requires API v126+


      // WiFiClient is required by PubSubClient and HTTP POST

      int status;

      int last_mqtt_reconnect;

      bool shouldSaveConfig;
      bool connected;

      bool once;

      StaticJsonBuffer<1024> jsonBuffer;

      void configModeCallback(EAVManager*);
      void saveConfigCallback();
      void esp_update(String);
      void thinx_parse(String);
      String thinx_mqtt_channel();
      String thinx_mqtt_shared_channel();
      String thinx_mac();
      void checkin();
      void senddata(String);
      void mqtt_callback(const MQTT::Publish&);
      bool start_mqtt(WiFiClient);
      void thinx_mqtt_callback(char*, byte*, unsigned int);
      void connect();
      bool restoreDeviceInfo();
      void saveDeviceInfo();
      String deviceInfo();
};

#endif
