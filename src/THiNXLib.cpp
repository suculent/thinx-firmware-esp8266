#include "THiNXLib.h"

#ifndef UNIT_TEST  // IMPORTANT LINE!

extern "C" {
  #include "user_interface.h"
  #include "thinx.h"
  #include <cont.h>
  extern cont_t g_cont;
}

register uint32_t *sp asm("a1");

THiNX::THiNX() {
}

THiNX::THiNX(const char * __apikey) {

  Serial.print("\nTHiNXLib rev");
  Serial.println(String(THX_REVISION)); // should be generated using platformio.ini pre-build

  // see lines ../hardware/cores/esp8266/Esp.cpp:80..100
  wdt_disable(); // causes wdt reset after 8 seconds!
  wdt_enable(65535); // must be called from wdt_disable() state!

  status = WL_IDLE_STATUS;
  once = true;
  should_save_config = false;
  connected = false;

  mqtt_client = NULL;

  if (once != true) {
    once = true;
  }

  checked_in = false;
  mqtt_payload = "";
  mqtt_result = false;
  mqtt_connected = false;

  wifi_connection_in_progress = false;
  thx_wifi_client = new WiFiClient();

  thinx_udid = strdup("");
  app_version = strdup("");
  available_update_url = strdup("");
  thinx_cloud_url = strdup("thinx.cloud");
  thinx_commit_id = strdup("");
  thinx_firmware_version_short = strdup("");
  thinx_firmware_version = strdup("");
  thinx_mqtt_url = strdup("thinx.cloud");
  thinx_version_id = strdup("");
  thinx_owner = strdup("");
  thinx_api_key = strdup("");

  wifi_retry = 0;

#ifdef __USE_WIFI_MANAGER__
  manager = new WiFiManager;
  api_key_param = new WiFiManagerParameter("apikey", "API Key", thinx_api_key, 64);
  manager->addParameter(api_key_param);
  manager->setTimeout(5000);
  manager->setDebugOutput(false); // does some logging on mode set

  // TODO: FIXME:
  // manager->setSaveConfigCallback(  saveConfigCallback );

  /*
  lib/thinx-lib-esp8266-arduino/src/THiNXLib.cpp:64:55: error: no matching function for call to 'WiFiMana
  ger::setSaveConfigCallback(<unresolved overloaded function type>)'
  manager->setSaveConfigCallback(  saveConfigCallback );
  ^
  lib/thinx-lib-esp8266-arduino/src/THiNXLib.cpp:64:55: note: candidate is:
  In file included from lib/thinx-lib-esp8266-arduino/src/THiNXLib.h:9:0,
  from lib/thinx-lib-esp8266-arduino/src/THiNXLib.cpp:1:
  lib/WiFiManager/WiFiManager.h:101:19: note: void WiFiManager::setSaveConfigCallback(void (*)())
  void          setSaveConfigCallback( void (*func)(void) );
  ^
  lib/WiFiManager/WiFiManager.h:101:19: note:   no known conversion for argument 1 from '<unresolved over
  loaded function type>' to 'void (*)()'
  */

#else
  if (WiFi.status() == WL_CONNECTED) {
    connected = true;
    wifi_connection_in_progress = false;
  }
#endif

  EEPROM.begin(512); // should be SPI_FLASH_SEC_SIZE

  import_build_time_constants();

  if (strlen(thinx_api_key) > 4) {
    //Serial.print("*TH: Init with stored API Key: ");
  } else {
    if (strlen(__apikey) > 4) {
      Serial.print("*TH: With custom API Key: ");
      thinx_api_key = strdup(__apikey);
      Serial.println(thinx_api_key);
    } else {
      Serial.println("*TH: Init without AK (captive portal)...");
    }
  }
  initWithAPIKey(thinx_api_key);
}

// Designated initializer
void THiNX::initWithAPIKey(const char * __apikey) {

  bool success = restore_device_info();

  Serial.println("*TH: Device info restored.");

  // FS may deprecate in favour of EEPROM
#ifdef __USE_SPIFFS__
  Serial.println("*TH: Checking FS...");
  if (!fsck()) {
    Serial.println("*TH: Filesystem check failed, disabling THiNX.");
    return;
  }
#endif

  if (strlen(thinx_api_key) < 4) {
    if (String(__apikey).length() > 1) {
      thinx_api_key = strdup(__apikey);
    }
  }
}

void THiNX::connect() {

  if (connected) {
    Serial.println("*TH: connected");
    return;
  }

  Serial.print("*TH: connecting: "); Serial.println(wifi_retry);

  if (WiFi.SSID()) {


    if (!wifi_connection_in_progress) {

      Serial.print("*TH: SSID "); Serial.println(WiFi.SSID());

      if (WiFi.getMode() == WIFI_AP) {

        Serial.print("THiNX > LOOP > START() > AP SSID");
        Serial.print(WiFi.SSID());

      } else {

        Serial.println("*TH: LOOP > CONNECT > STATION DISCONNECT");
        ETS_UART_INTR_DISABLE();
        wifi_station_disconnect();
        ETS_UART_INTR_ENABLE();
        Serial.println("*TH: LOOP > CONNECT > STATION RECONNECT");
        //WiFi.begin(THINX_ENV_SSID, THINX_ENV_PASS);
        WiFi.begin();

      }

      wifi_connection_in_progress = true; // prevents re-entering connect_wifi(); should timeout
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("THiNX > LOOP > ALREADY CONNECTED");
    connected = true; // prevents re-entering start() [this method]
    wifi_connection_in_progress = false;
  } else {
    Serial.println("THiNX > LOOP > CONNECTING WiFi:");
    connect_wifi();
  }
}

/*
 * Connection
 */
void THiNX::connect_wifi() {

   //Serial.printf("autoConnect: unmodified stack   = %4d\n", cont_get_free_stack(&g_cont));
   //Serial.printf("autoConnect: current free stack = %4d\n", 4 * (sp - g_cont.stack));
   // 4, 208!

#ifdef __USE_WIFI_MANAGER__
   Serial.setDebugOutput(true);
   manager->setDebugOutput(true); // does some logging on mode set
   Serial.println("*TH: AutoConnecting..."); Serial.flush();
   //delay(1);
   while ( !manager->autoConnect("AP-THiNX", "PASSWORD") ) {
     Serial.println("*TH: AutoConnect loop...");
     delay(1);
   }
#else
  if (connected) {
    return;
  }

  wifi_retry++;

  //Serial.printf("THiNXLib::connect_wifi(): unmodified stack   = %4d\n", cont_get_free_stack(&g_cont));
  //Serial.printf("THiNXLib::connect_wifi(): current free stack = %4d\n", 4 * (sp - g_cont.stack));
  //Serial.print("*THiNXLib::connect_wifi(SKIP): heap = "); Serial.println(system_get_free_heap_size());

  // 84, 176; 35856

  // reported to crash at 1000!
  if (wifi_retry > 1000) {
    Serial.printf("*TH: WiFi Retry timeout.");
    wifi_connection_in_progress = false;
    WiFi.disconnect();
    Serial.println("*TH: Starting THiNX-AP with PASSWORD...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("THiNX-AP", "PASSWORD");
    wifi_retry = 0;
    connection_in_progress = false;
    return;
  }

  if (wifi_connection_in_progress) {
    Serial.println("*TH: Connection in progress...");
    return;
  }

  if (strlen(THINX_ENV_SSID) > 2) {
    if (wifi_retry == 0) {
      Serial.println("*TH: Connecting to AP with pre-defined credentials...");
      WiFi.mode(WIFI_STA);
      WiFi.begin(strdup(THINX_ENV_SSID), strdup(THINX_ENV_PASS));
      wifi_connection_in_progress = true; // prevents re-entering connect_wifi()
    }
    // exit loop here to prevent wdt lock
  }
#endif
 }

 void THiNX::checkin() {
   Serial.println("*TH: Starting API checkin...");
   if(!connected) {
     Serial.println("*TH: Cannot checkin while not connected, exiting.");
   } else {
     senddata(checkin_body());
   }
 }

 String THiNX::checkin_body() {

   //Serial.println("*TH: Building request...");
   //Serial.print("*THiNXLib::checkin_body(): heap = ");
   //Serial.println(system_get_free_heap_size());

   JsonObject& root = jsonBuffer.createObject();
   root["mac"] = thinx_mac();
   root["firmware"] = THINX_FIRMWARE_VERSION;
   root["version"] = THINX_FIRMWARE_VERSION_SHORT;
   root["commit"] = THINX_COMMIT_ID;

   root["owner"] = thinx_owner;
   root["alias"] = thinx_alias;

   if (strlen(thinx_udid) > 4) {
     root["udid"] = thinx_udid;
   }

   root["platform"] = THINX_PLATFORM;

   // Serial.println("*TH: Wrapping request..."); OK until here...

   JsonObject& wrapper = wrapperBuffer.createObject();
   wrapper["registration"] = root;

 #ifdef __DEBUG_JSON__
   wrapper.printTo(Serial);
   Serial.println();
 #endif

   String body;
   wrapper.printTo(body);
   return body;
 }

void THiNX::senddata(String body) {

  if (thx_wifi_client->connect(thinx_cloud_url, 7442)) {
    Serial.println("*THiNXLib::senddata(): with api key...");

    thx_wifi_client->println("POST /device/register HTTP/1.1");
    thx_wifi_client->print("Host: "); thx_wifi_client->println(thinx_cloud_url);
    thx_wifi_client->print("Authentication: "); thx_wifi_client->println(thinx_api_key);
    thx_wifi_client->println("Accept: application/json"); // application/json
    thx_wifi_client->println("Origin: device");
    thx_wifi_client->println("Content-Type: application/json");
    thx_wifi_client->println("User-Agent: THiNX-Client");
    thx_wifi_client->print("Content-Length: ");
    thx_wifi_client->println(body.length());
    thx_wifi_client->println();
    //Serial.println("Headers set...");
    thx_wifi_client->println(body);
    //Serial.println("Body sent...");

    long interval = 10000;
    unsigned long currentMillis = millis(), previousMillis = millis();

    Serial.print("*THiNXLib::senddata(): waiting for response...");
    // TODO: FIXME: Drop the loop here, wait for response!

    // Wait until client available or timeout...
    while(!thx_wifi_client->available()){
      delay(1);
      if( (currentMillis - previousMillis) > interval ){
        //Serial.println("Response Timeout. TODO: Should retry later.");
        thx_wifi_client->stop();
        return;
      }
      currentMillis = millis();
    }

    // Read while connected
    String payload = "";
    while ( thx_wifi_client->connected() ) {
      delay(1);
      if ( thx_wifi_client->available() ) {
        char str = thx_wifi_client->read();
        payload = payload + String(str);
      }
    }

    Serial.print("*THiNXLib::senddata(): parsing payload...");
    parse(payload);

  } else {
    Serial.println("*TH: API connection failed.");
    return;
  }
}

/*
 * Response Parser
 */

void THiNX::parse(String payload) {

  // TODO: Should parse response only for this device_id (which must be internal and not a mac)

  payload_type ptype = Unknown;

  int startIndex = 0;
  int endIndex = payload.length();

  int reg_index = payload.indexOf("{\"registration\"");
  int upd_index = payload.indexOf("{\"update\"");
  int not_index = payload.indexOf("{\"notification\"");

  if (upd_index > startIndex) {
    startIndex = upd_index;
    ptype = UPDATE;
  }

  if (reg_index > startIndex) {
    startIndex = reg_index;
    endIndex = payload.indexOf("}}") + 2;
    ptype = REGISTRATION;
  }

  if (not_index > startIndex) {
    startIndex = not_index;
    endIndex = payload.indexOf("}}") + 2; // is this still needed?
    ptype = NOTIFICATION;
  }

  String body = payload.substring(startIndex, endIndex);

#ifdef __DEBUG__
    Serial.print("*TH: Parsing response: '");
    Serial.print(body);
    Serial.println("'");
#endif

  JsonObject& root = jsonBuffer.parseObject(body.c_str());

  if ( !root.success() ) {
  Serial.println("Failed parsing root node.");
    return;
  }

  switch (ptype) {

    case UPDATE: {

      JsonObject& update = root["update"];
      Serial.println("TODO: Parse update payload...");

      // Parse update (work in progress)
      String mac = update["mac"];
      String this_mac = String(thinx_mac());
      Serial.println(String("mac: ") + mac);

      if (!mac.equals(this_mac)) {
        Serial.println("*TH: Warning: firmware is dedicated to device with different MAC.");
      }

      // Check current firmware based on commit id and store Updated state...
      String commit = update["commit"];
      Serial.println(String("commit: ") + commit);

      // Check current firmware based on version and store Updated state...
      String version = update["version"];
      Serial.println(String("version: ") + version);

      if ((commit == thinx_commit_id) && (version == thinx_version_id)) {
        if (strlen(available_update_url) > 5) {
          Serial.println("*TH: firmware has same commit_id as current and update availability is stored. Firmware has been installed.");
          available_update_url = "";
          save_device_info();
          notify_on_successful_update();
          return;
        } else {
          Serial.println("*TH: Info: firmware has same commit_id as current and no update is available.");
        }
      }

      // In case automatic updates are disabled,
      // we must ask user to commence firmware update.
      if (THINX_AUTO_UPDATE == false) {
        if (mqtt_client) {
          mqtt_client->publish(
            thinx_mqtt_channel(),
            "{ title: \"Update Available\", body: \"There is an update available for this device. Do you want to install it now?\", type: \"actionable\", response_type: \"bool\" }"
          );
        }

      } else {

        Serial.println("Starting update...");

        // FROM LUA: update variants
        // local files = payload['files']
        // local ott   = payload['ott']
        // local url   = payload['url']
        // local type  = payload['type']

        String type = update["type"];
        Serial.print("Payload type: "); Serial.println(type);

        String files = update["files"];

        String url = update["url"]; // may be OTT URL
        available_update_url = url.c_str();

        String ott = update["ott"];
        available_update_url = ott.c_str();

        save_device_info();

        if (url) {
          Serial.println("*TH: Force update URL must not contain HTTP!!! :" + url);
          url.replace("http://", "");
          // TODO: must not contain HTTP, extend with http://thinx.cloud/"
          // TODO: Replace thinx.cloud with thinx.local in case proxy is available
          update_and_reboot(url);
        }
        return;
      }

    } break;

    case NOTIFICATION: {

      // Currently, this is used for update only, can be extended with request_category or similar.
      JsonObject& notification = root["notification"];

      if ( !notification.success() ) {
        Serial.println("Failed parsing notification node.");
        return;
      }

      String type = notification["response_type"];
      if ((type == "bool") || (type == "boolean")) {
        bool response = notification["response"];
        if (response == true) {
          Serial.println("User allowed update using boolean.");
          if (strlen(available_update_url) > 4) {
            update_and_reboot(available_update_url);
          }
        } else {
          Serial.println("User denied update using boolean.");
        }
      }

      if ((type == "string") || (type == "String")) {
        String response = notification["response"];
        if (response == "yes") {
          Serial.println("User allowed update using string.");
          if (strlen(available_update_url) > 4) {
            update_and_reboot(available_update_url);
          }
        } else if (response == "no") {
          Serial.println("User denied update using string.");
        }
      }

    } break;

    case REGISTRATION: {

      JsonObject& registration = root["registration"];

      if ( !registration.success() ) {
        Serial.println("Failed parsing registration node.");
        return;
      }

      bool success = registration["success"];
      String status = registration["status"];

      if (status == "OK") {

        String alias = registration["alias"];
        if ( alias.length() > 0 ) {
          thinx_alias = strdup(alias.c_str());
        }

        String owner = registration["owner"];
        if ( owner.length() > 0 ) {
          thinx_owner = strdup(owner.c_str());
        }

        String udid = registration["udid"];
        if ( udid.length() > 4 ) {
          thinx_udid = strdup(udid.c_str());
        }

        // Check current firmware based on commit id and store Updated state...
        String commit = registration["commit"];
        Serial.print("commit: "); Serial.println(commit);

        // Check current firmware based on version and store Updated state...
        String version = registration["version"];
        Serial.print("version: "); Serial.println(version);

        if ((commit == thinx_commit_id) && (version == thinx_version_id)) {
          if (strlen(available_update_url) > 4) {
            Serial.println("*TH: firmware has same commit_id as current and update availability is stored. Firmware has been installed.");
            available_update_url = "";
            save_device_info();
            notify_on_successful_update();
            return;
          } else {
            Serial.println("*TH: Info: firmware has same commit_id as current and no update is available.");
          }
        }

        save_device_info();

      } else if (status == "FIRMWARE_UPDATE") {

        String mac = registration["mac"];
        Serial.println(String("mac: ") + mac);
        // TODO: must be current or 'ANY'

        String commit = registration["commit"];
        Serial.println(String("commit: ") + commit);

        // should not be same except for forced update
        if (commit == thinx_commit_id) {
          Serial.println("*TH: Warning: new firmware has same commit_id as current.");
        }

        String version = registration["version"];
        Serial.println(String("version: ") + version);

        Serial.println("Starting update...");

        String url = registration["url"];
        if (url) {
          Serial.println("*TH: Running update with URL that should not contain http! :" + url);
          url.replace("http://", "");
          update_and_reboot(url);
        }
      }

      } break;

    default:
      Serial.println("Nothing to do...");
      break;
  }

}

/*
 * MQTT
 */

const char* THiNX::thinx_mqtt_channel() {
  String s = String("/") + String(thinx_owner) + String("/") + String(thinx_udid);
  return s.c_str();
}

const char* THiNX::thinx_mqtt_status_channel() {
  String s = String("/") + String(thinx_owner) + String("/") + String(thinx_udid) + String("/status");
 return s.c_str();
}

const char * THiNX::thinx_mac() {
 byte mac[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 };
 WiFi.macAddress(mac);
 sprintf(mac_string, "5CCF7F%6X", ESP.getChipId()); // ESP8266 only!
 /*
#ifdef __ESP32__
 sprintf(mac_string, "5CCF7C%6X", ESP.getChipId()); // ESP8266 only!
#endif
#ifdef __ESP8266__
 sprintf(mac_string, "5CCF7F%6X", ESP.getChipId()); // ESP8266 only!
#endif
 */
 return mac_string;
}

void THiNX::publish() {
  if (!connected) return;
  if (mqtt_client == NULL) return;
  if (strlen(thinx_udid) < 4) return;
  const char * channel = thinx_mqtt_status_channel();
  String response = "{ \"status\" : \"connected\" }";
  if (mqtt_client->connected()) {
    Serial.println("*TH: MQTT connected...");
    mqtt_client->publish(channel, response.c_str());
    mqtt_client->loop();
  } else {
    Serial.println("*TH: MQTT not connected, reconnecting...");
    mqtt_result = start_mqtt();
    if (mqtt_result && mqtt_client->connected()) {
      mqtt_client->publish(channel, response.c_str());
      Serial.println("*TH: MQTT reconnected, published default message.");
    } else {
      Serial.println("*TH: MQTT Reconnect failed...");
    }
  }
}

void THiNX::notify_on_successful_update() {
  if (mqtt_client) {
    mqtt_client->publish(
      thinx_mqtt_status_channel(),
      "{ title: \"Update Successful\", body: \"The device has been successfully updated.\", type: \"success\" }"
    );
  } else {
    Serial.println("Device updated but MQTT not active to notify. TODO: Store.");
  }
}

bool THiNX::start_mqtt() {

  if (mqtt_client != NULL) {
    if (mqtt_client->connected()) {
      return true;
    } else {
      return false;
    }
  }

  if (strlen(thinx_udid) < 4) {
    return false;
  }

  Serial.print("*TH: UDID: ");
  Serial.println(thinx_udid);

  Serial.print("*TH: Contacting MQTT server ");
  Serial.println(thinx_mqtt_url);

  //PubSubClient mqtt_client(thx_wifi_client, thinx_mqtt_url.c_str());
  Serial.print("*TH: Starting client");
  if (mqtt_client == NULL) {
    mqtt_client = new PubSubClient(*thx_wifi_client, thinx_mqtt_url);
  }

  Serial.print(" on port ");
  Serial.println(thinx_mqtt_port);

  last_mqtt_reconnect = 0;

  Serial.println("*TH: Connecting to MQTT...");

  Serial.print("*TH: AK: ");
  Serial.println(thinx_api_key);
  Serial.print("*TH: DCH: ");
  Serial.println(thinx_mqtt_channel());

  const char* id = thinx_mac();
  const char* user = thinx_udid;
  const char* pass = thinx_api_key;
  const char* willTopic = thinx_mqtt_status_channel();
  int willQos = 0;
  bool willRetain = false;

  delay(1);

  if (mqtt_client->connect(MQTT::Connect(id)
                .set_will(willTopic, "{ \"status\" : \"disconnected\" }")
                .set_auth(user, pass)
                .set_keepalive(30)
              )) {

        mqtt_connected = true;

        return true;

      } else {

        Serial.println("*TH: MQTT Not connected.");
        return false;

      }

      mqtt_client->set_callback([this](const MQTT::Publish &pub){

        if (pub.has_stream()) {
          Serial.println("*TH: MQTT Type: Stream...");
          uint32_t startTime = millis();
          uint32_t size = pub.payload_len();
          if ( ESP.updateSketch(*pub.payload_stream(), size, true, false) ) {

            // Notify on reboot for update
            mqtt_client->publish(
              thinx_mqtt_status_channel(),
              "{ \"status\" : \"rebooting\" }"
            );
            mqtt_client->disconnect();
            pub.payload_stream()->stop();
            Serial.printf("Update Success: %u\nRebooting...\n", millis() - startTime);
            ESP.restart();
          }

          Serial.println("stop.");

        } else {

          Serial.println("*TH: MQTT Type: String or JSON...");
          Serial.println(pub.payload_string());
          parse(pub.payload_string());

        }
    }); // end-of-callback
}

#ifdef __USE_WIFI_MANAGER__
/*
 * WiFiManager Setup Callback

void THiNX::saveConfigCallback() {
  should_save_config = true;
  strcpy(thx_api_key, api_key_param->getValue());
}
*/
#endif

/*
 * Device Info
 */

// Calles (private): initWithAPIKey; save_device_info()
bool THiNX::restore_device_info() {

#ifndef __USE_SPIFFS__
  // Serial.println("*TH: restoring configuration from EEPROM...");
  int value;
  long buf_len = 512;
  char info[512] = {0};

  for (long a = 0; a < buf_len; a++) {
    value = EEPROM.read(a);
    if (value == 0) {
      Serial.print("*TH: "); Serial.print(a); Serial.print(" bytes read from EEPROM.");
      break;
    }
    // validate at least data start
    if (a == 0) {
      if (value != '{') {
        Serial.println("Data is not a JSON string, exiting.");
        break;
      }
    }
    info[a] = value;
  }

  String data = String(info) + "\n"; // \n helps the JSON parser not to crash
  //Serial.println("'"+data+"'");

#else
  if (!SPIFFS.exists("/thx.cfg")) {
    Serial.println("*TH: No persistent data found.");
    return false;
  }
   File f = SPIFFS.open("/thx.cfg", "r");
   Serial.println("*TH: Found persistent data...");
   if (!f) {
       Serial.println("*TH: No remote configuration found so far...");
       return false;
   }
   if (f.size() == 0) {
        Serial.println("*TH: Remote configuration file empty...");
       return false;
   }
   String data = f.readStringUntil('\n');
#endif

   Serial.println("*TH: Parsing JSON...");

   JsonObject& config = jsonBuffer.parseObject(data.c_str());
   if (!config.success()) {
     Serial.println("*TH: parsing JSON failed.");
     return false;
   } else {

    Serial.println("*TH: Parsing saved data..."); // may crash: Serial.flush();
    Serial.print("'");
    Serial.print(data);
    Serial.println("'");

     const char* alias = config["alias"];
     if (strlen(alias) > 1) {
       thinx_alias = strdup(alias);
     }
     const char* owner = config["owner"];
     if (strlen(owner) > 4) {
       thinx_owner = strdup(owner);
     }
     const char* apikey = config["apikey"];
     if (strlen(apikey) > 8) {
      thinx_api_key = strdup(apikey);
      sprintf(thx_api_key, "%s", apikey); // 40 max; copy; should deprecate
     }
     const char* update = config["update"];
     if (strlen(update) > 4) {
       available_update_url = strdup(update);
     }
     const char* udid = config["udid"];
     if ((strlen(udid) > 4)) {
      thinx_udid = strdup(udid);
     } else {
      thinx_udid = strdup(THINX_UDID);
     }
#ifdef __USE_SPIFFS__
    Serial.print("*TH: Closing SPIFFS file.");
    f.close();
#endif
   }
   return true;
 }

 /* Stores mutable device data (alias, owner) retrieved from API */
 void THiNX::save_device_info()
 {
   String info = deviceInfo() + "\n";

   // disabled for it crashes when closing the file (LoadStoreAlignmentCause) when using String
#ifdef __USE_SPIFFS__
   File f = SPIFFS.open("/thx.cfg", "w");
   if (f) {
     Serial.println("*TH: saving configuration to SPIFFS...");
     f.println(info); // String instead of const char* due to LoadStoreAlignmentCause...
     Serial.println("*TH: closing file...");
     f.close();
     delay(1); // yield some cpu time for saving
   }
#else
  Serial.println("*TH: saving configuration to EEPROM...");
  Serial.println(info);
  for (long addr = 0; addr <= info.length(); addr++) {
    EEPROM.put(addr, info.charAt(addr));
  }
  EEPROM.commit();
  Serial.println("*TH: EEPROM data committed..."); // works until here so far...
#endif
}

String THiNX::deviceInfo() {

  //Serial.println("*TH: building device info:");

  JsonObject& root = jsonBuffer.createObject();
  root["alias"] = thinx_alias; // allow alias change
  root["owner"] = thinx_owner; // allow owner change
  root["update"] = available_update_url; // allow update
  root["apikey"] = thinx_api_key; // allow changing API Key
  root["udid"] = thinx_udid; // allow setting UDID

  //Serial.print("*TH: thinx_alias: ");
  //Serial.println(thinx_alias);

  //Serial.print("*TH: thinx_owner: ");
  //Serial.println(thinx_owner);

  //Serial.print("*TH: thinx_api_key: ");
  //Serial.println(thinx_api_key);

  //Serial.print("*TH: thinx_udid: ");
  //Serial.println(thinx_udid);

  //Serial.print("*TH: available_update_url: ");
  //Serial.println(available_update_url);

  String jsonString;
  root.printTo(jsonString);

  return jsonString;
}


/*
 * Updates
 */

// update_file(name, data)
// update_from_url(name, url)

void THiNX::update_and_reboot(String url) {

#ifdef __DEBUG__
  Serial.println("[update] Starting update & reboot...");
#endif

// #define __USE_ESP_UPDATER__ ; // Warning, this is MQTT-based streamed update!
#ifdef __USE_ESP_UPDATER__
  uint32_t size = pub.payload_len();
  if (ESP.updateSketch(*pub.payload_stream(), size, true, false)) {
    Serial.println("Clearing retained message.");
    mqtt_client->publish(MQTT::Publish(pub.topic(), "").set_retain());
    mqtt_client->disconnect();

    Serial.printf("Update Success: %u\nRebooting...\n", millis() - startTime);

    // Notify on reboot for update
    if (mqtt_client) {
      mqtt_client->publish(
        thinx_mqtt_status_channel(),
        thx_reboot_response.c_str()
      );
      mqtt_client->disconnect();
    }

    ESP.restart();
  }
#else

  //
  t_httpUpdate_return ret = ESPhttpUpdate.update(thinx_cloud_url, 80, url.c_str());

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
#endif
}

/* Imports all required build-time values from thinx.h */
void THiNX::import_build_time_constants() {

  // Only if not overridden by user
  if (strlen(thinx_api_key) < 4) {
    thinx_api_key = strdup(THINX_API_KEY);
  }

  thinx_udid = strdup(THINX_UDID);
  thinx_commit_id = strdup(THINX_COMMIT_ID);
  thinx_mqtt_url = strdup(THINX_MQTT_URL);
  thinx_cloud_url = strdup(THINX_CLOUD_URL);
  thinx_alias = strdup(THINX_ALIAS);
  thinx_owner = strdup(THINX_OWNER);
  thinx_mqtt_port = THINX_MQTT_PORT;
  thinx_api_port = THINX_API_PORT;
  thinx_auto_update = THINX_AUTO_UPDATE;
  thinx_forced_update = THINX_FORCED_UPDATE;
  thinx_firmware_version = strdup(THINX_FIRMWARE_VERSION);
  thinx_firmware_version_short = strdup(THINX_FIRMWARE_VERSION_SHORT);
  app_version = strdup(THINX_APP_VERSION);
#ifdef __DEBUG__
  Serial.println("*TH: Imported build-time constants...");
#endif
}

bool THiNX::fsck() {
  String realSize = String(ESP.getFlashChipRealSize());
  String ideSize = String(ESP.getFlashChipSize());
  bool flashCorrectlyConfigured = realSize.equals(ideSize);
  bool fileSystemReady = false;
  if(flashCorrectlyConfigured) {
    Serial.println("* TH: Starting SPIFFS...");
    fileSystemReady = SPIFFS.begin();
    if (!fileSystemReady) {
      Serial.println("* TH: Formatting SPIFFS...");
      fileSystemReady = SPIFFS.format();;
      Serial.println("* TH: Format complete, rebooting..."); Serial.flush();
      ESP.restart();
      return false;
    }
    Serial.println("* TH: SPIFFS Initialization completed.");
  }  else {
    Serial.println("flash incorrectly configured, SPIFFS cannot start, IDE size: " + ideSize + ", real size: " + realSize);
  }

  return fileSystemReady ? true : false;
}

void THiNX::evt_save_api_key() {
  if (should_save_config) {
    if (strlen(thx_api_key) > 4) {
      thinx_api_key = thx_api_key;
      Serial.print("Saving thx_api_key from Captive Portal: ");
      Serial.println(thinx_api_key);
      save_device_info();
      should_save_config = false;
    }
  }
}

/*
 * Core loop
 */

void THiNX::loop() {

  //uint32_t memfree = system_get_free_heap_size(); Serial.print("THINX LOOP memfree                  = "); Serial.println(memfree);
  //Serial.printf("THiNXLib::connect_wifi(): unmodified stack   = %4d\n", cont_get_free_stack(&g_cont));
  //Serial.printf("THiNXLib::connect_wifi(): current free stack = %4d\n", 4 * (sp - g_cont.stack));

  // Serial.print("*THiNXLib::connect_wifi(SKIP): heap = "); Serial.println(system_get_free_heap_size());

  //
  // If not connected, start connection in progress...
  //

  if (!connected) {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
    } else {
      connect(); // blocking
    }
  } else {

    //
    // After MQTT gets connected:
    //

    if (mqtt_connected && mqtt_client->connected()) {
      // disable looping... should be done with something like mqtt_checked_in = true instead
      mqtt_connected = false;

      Serial.println("*TH: MQTT Publishing device status... ");
      // Publish status on status channel
      mqtt_client->publish(
        thinx_mqtt_status_channel(),
        "{ \"status\" : \"connected\" }"
      );

      Serial.println("*TH: MQTT Subscribing device channel...");
      if (mqtt_client->subscribe(thinx_mqtt_channel())) {
        Serial.print("*TH: DCH ");
        Serial.print(thinx_mqtt_channel());
        Serial.println(" successfully subscribed.");
        Serial.print("*THiNXLib::connect_wifi(SKIP): heap = "); Serial.println(system_get_free_heap_size());
        finalize();
      }
    }

    //
    // After checked in, connect MQTT
    //

    if (checked_in == true) {
      if (mqtt_client != NULL) {
        mqtt_client->loop();
      } else {
        Serial.println("*TH: WiFi connected, starting MQTT...");
        delay(1);
        mqtt_result = start_mqtt(); // requires valid udid and api_keys, and allocated WiFiClient; might be blocking
      }
    }

    //
    // If connected and not checked_in, perform check in.
    //

    if (checked_in == false) {
      if (strlen(thinx_api_key) > 4) {
        Serial.println("*TH: WiFi connected, checking in...");
        checked_in = true;
        checkin(); // blocking
        return; // init MQTT in next loop
      }
    }

    // Save API key on change
    evt_save_api_key(); // only happens on should_save_config
  }
}

void THiNX::setFinalizeCallback( void (*func)(void) ) {
  _finalize_callback = func;
}

void THiNX::finalize() {
  Serial.println("*TH: Checkin completed.");
  Serial.print("*THiNXLib::finalize heap = "); Serial.println(system_get_free_heap_size());
  if (_finalize_callback) {
    _finalize_callback();
  }
}

#endif    // IMPORTANT LINE!
