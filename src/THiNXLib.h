#include <Arduino.h>

#define __DEBUG__
#define __DEBUG_JSON__

#include <stdio.h>
#include "ArduinoJson/ArduinoJson.h"

#include <FS.h>
#include "EAVManager/EAVManager.h"
//#include <EAVManager.h>

// Using better than Arduino-bundled version of MQTT https://github.com/Imroy/pubsubclient
#include "PubSubClient/PubSubClient.h" // Local checkout
//#include <PubSubClient.h> // Arduino Library

// TODO: Add UDP AT&U= responder like in EAV? Considered unsafe. Device will notify available update and download/install it on its own (possibly throught THiNX Security Gateway (THiNX )
// IN PROGRESS: Add MQTT client (target IP defined using Thinx.h) and forced firmware update responder (will update on force or save in-memory state from new or retained mqtt notification)
// TODO: Add UDP responder AT&U only to update to next available firmware (from save in-memory state)

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#define MQTT_BUFFER_SIZE 512

class THiNX {

  public:

    THiNX();
    THiNX(const char *);

    enum payload_type {
      Unknown = 0,
      UPDATE = 1,		// Firmware Update Response Payload
      REGISTRATION = 2,		// Registration Response Payload
      NOTIFICATION = 3, // Notification/Interaction Response Payload
      Reserved = 255,		// Reserved
    };

    // Public API
    void initWithAPIKey(const char *);
    void publish();
    void loop();

    // Internal public API
    String thx_connected_response = "{ \"status\" : \"connected\" }";
    String thx_disconnected_response = "{ \"status\" : \"disconnected\" }";
    String thx_reboot_response = "{ \"status\" : \"rebooting\" }";
    String thx_update_question = "{ title: \"Update Available\", body: \"There is an update available for this device. Do you want to install it now?\", type: \"actionable\", response_type: \"bool\" }";

    String checkin_body();

    // WiFi Client
    EAVManager *manager;
    EAVManagerParameter *api_key_param;

    // MQTT

    PubSubClient *mqtt_client;

    uint8_t buf[MQTT_BUFFER_SIZE];

    String thinx_mqtt_channel();
    String thinx_mqtt_status_channel();

    // Response parsers
    //void parse_registration(JSONObject);
    //void parse_update(JSONObject);

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
    char* thinx_api_key;

    private:

      // WiFi Manager
      WiFiClient *thx_wifi_client;
      int status;                 // global WiFi status
      bool once;
      bool connected;
      void saveConfigCallback();

      // THiNX API
      char thx_api_key[64];     // new firmware requires 64 bytes
      char thx_udid[64];        // new firmware requires 64 bytes

      StaticJsonBuffer<1024> jsonBuffer;
      StaticJsonBuffer<2048> wrapperBuffer;

      void checkin();
      void senddata(String);
      void parse(String);
      bool connect();
      void update_and_reboot(String);

      // MQTT
      int last_mqtt_reconnect;
      bool start_mqtt();
      bool mqtt_result;

      String thinx_mqtt_shared_channel();
      String thinx_mac();

      // Data Storage
      void import_build_time_constants();

      bool shouldSaveConfig;
      bool restore_device_info();
      void save_device_info();
      String deviceInfo();

      // Updates
      void notify_on_successful_update();

      // File system consistency check
      bool fsck();
};
