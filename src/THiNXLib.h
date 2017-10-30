#include <Arduino.h>

#define __DEBUG__
#define __DEBUG_JSON__

#define __USE_WIFI_MANAGER__
//#define __USE_SPIFFS__

#ifdef __USE_WIFI_MANAGER__
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#endif

#include <stdio.h>
#include <FS.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include "ArduinoJson/ArduinoJson.h"

// Using better than Arduino-bundled version of MQTT https://github.com/Imroy/pubsubclient
#include "PubSubClient/PubSubClient.h" // Local checkout
//#include <PubSubClient.h> // Arduino Library

#ifndef THX_REVISION
  #ifdef THINX_FIRMWARE_VERSION_SHORT
    #define THX_REVISION THINX_FIRMWARE_VERSION_SHORT
  #endif
#endif

class THiNX {

  public:

    #ifdef __USE_WIFI_MANAGER__
        static WiFiManagerParameter *api_key_param;
        static WiFiManagerParameter *owner_param;
        static int should_save_config;                // after autoconnect, may provide new API Key
        static void saveConfigCallback();
    #endif

    THiNX();
    THiNX(const char *, const char *); // (const char * __apikey, const char * __owner_id)
    THiNX(const char *);  // (const char * __apikey)

    enum payload_type {
      Unknown = 0,
      UPDATE = 1,		                         // Firmware Update Response Payload
      REGISTRATION = 2,		                   // Registration Response Payload
      NOTIFICATION = 3,                      // Notification/Interaction Response Payload
      Reserved = 255,		                     // Reserved
    };

    // Public API
    void initWithAPIKey(const char *);
    void publish();
    void loop();

    String checkin_body();                  // TODO: Refactor to C-string

    // MQTT
    PubSubClient *mqtt_client;

    String thinx_mqtt_channel();
    char mqtt_device_channel[128]; //  = {0}
    String thinx_mqtt_status_channel();
    char mqtt_device_status_channel[128]; //  = {0}

    // Import build-time values from thinx.h
    const char* app_version;                  // max 80 bytes
    const char* available_update_url;         // up to 1k
    const char* thinx_cloud_url;              // up to 1k but generally something where FQDN fits
    const char* thinx_commit_id;              // 40 bytes + 1
    const char* thinx_firmware_version_short; // 14 bytes
    const char* thinx_firmware_version;       // max 80 bytes
    const char* thinx_mqtt_url;               // up to 1k but generally something where FQDN fits
    const char* thinx_version_id;             // max 80 bytes (DEPRECATED?)

    bool thinx_auto_update;
    bool thinx_forced_update;

    long thinx_mqtt_port;
    long thinx_api_port;

    // dynamic variables
    char* thinx_alias;
    char* thinx_owner;
    char* thinx_udid;

    bool connected;                         // WiFi connected in station mode

    void setFinalizeCallback( void (*func)(void) );

    int wifi_connection_in_progress;

    private:

      static char* thinx_api_key;
      static char* thinx_owner_key;

      //
      // Build-specific constants
      //

      #ifdef THX_REVISION
        const char* thx_revision = strdup(String(THX_REVISION).c_str());
      #else
        const char* thx_revision = "revision";
      #endif

      #ifdef THINX_COMMIT_ID
        const char* commit_id = THINX_COMMIT_ID;
      #else
        const char* commit_id = "commit-id";
      #endif

      #ifdef THINX_FIRMWARE_VERSION_SHORT
        const char* firmware_version_short = THINX_FIRMWARE_VERSION_SHORT;
      #else
        const char* firmware_version_short = "firmware-version-short";
      #endif

      //
      // THiNXLib
      //

      void configCallback();

      // WiFi Manager
      WiFiClient thx_wifi_client;
      int status;                             // global WiFi status
      bool once;                              // once token for initialization

      // THiNX API
      static char thx_api_key[65];            // static due to accesibility to WiFiManager
      static char thx_owner_key[65];          // static due to accesibility to WiFiManager

      char mac_string[17];
      const char * thinx_mac();

#ifndef __USE_SPIFFS__
      char json_info[512] = {0};           // statically allocated to prevent fragmentation (?)
#endif

      String json_output;

      // In order of appearance
      bool fsck();                            // check filesystem if using SPIFFS
      void connect();                         // start the connect loop
      void connect_wifi();                    // start connecting
      void checkin();                         // checkin when connected
      void senddata(String);
      void parse(String);
      void update_and_reboot(String);

      // MQTT
      bool start_mqtt();                      // connect to broker and subscribe
      bool mqtt_result;                       // success or failure on connection
      bool mqtt_connected;                    // success or failure on subscription
      String mqtt_payload;                    // mqtt_payload store for parsing
      int last_mqtt_reconnect;                // interval
      bool perform_mqtt_checkin;              // one-time flag
      bool all_done;                              // finalize flag

      // Data Storage
      void import_build_time_constants();     // sets variables from thinx.h file
      void save_device_info();                // saves variables to SPIFFS or EEPROM
      void restore_device_info();             // reads variables from SPIFFS or EEPROM
      void deviceInfo();                    // TODO: Refactor to C-string

      // Updates
      void notify_on_successful_update();     // send a MQTT notification back to Web UI

      // Event Queue / States
      bool checked_in;
      bool mqtt_started;
      bool complete;
      void evt_save_api_key();

      // Finalize
      void (*_finalize_callback)(void) = NULL;
      void finalize();                        // Complete the checkin, schedule, callback...

      // Local WiFi Impl
      bool wifi_wait_for_connect;
      unsigned long wifi_wait_start;
      unsigned long wifi_wait_timeout;
      int wifi_retry;
      uint8_t wifi_status;
};
