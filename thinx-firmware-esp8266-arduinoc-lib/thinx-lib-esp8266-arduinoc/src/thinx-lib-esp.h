#ifndef THiNX_H
#define THiNX_H

#include <Arduino.h>
#define __DEBUG__
#define __DEBUG_JSON__

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

class THiNX {

  public:

    THiNX(String);
    THiNX();

    // WiFi Client
    EAVManager *manager;
    EAVManagerParameter *api_key_param;

    // MQTT Client
    IPAddress mqtt_server;
    PubSubClient *mqtt_client;

    // THiNX Client
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

    private:

      // WiFi Manager
      const char* autoconf_ssid; // SSID in AP mode
      const char* autoconf_pwd; // fallback password, logged to console
      int status;                 // global WiFi status
      bool once;
      bool connected;
      void configModeCallback(EAVManager*);
      void saveConfigCallback();

      // THiNX API
      char thx_api_key[64];     // new firmware requires 64 bytes
      char thx_udid[64];        // new firmware requires 64 bytes
      StaticJsonBuffer<1024> jsonBuffer;

      void checkin();
      void senddata(String);
      void thinx_parse(String);
      void connect();
      void esp_update(String);

      // MQTT
      String thinx_mqtt_channel();
      String thinx_mqtt_shared_channel();
      String thinx_mac();
      int last_mqtt_reconnect;

      bool start_mqtt(WiFiClient);
      void mqtt_callback(const MQTT::Publish&);

      // Data Storage
      bool shouldSaveConfig;

      bool restoreDeviceInfo();
      void saveDeviceInfo();
      String deviceInfo();
};

#endif
