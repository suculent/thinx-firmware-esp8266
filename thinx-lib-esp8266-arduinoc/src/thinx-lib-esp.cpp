#include "thinx-lib-esp.h"

THiNX::THiNX(char* __thisOTAServer) {
  _thisOTAServer = __thisOTAServer;
  //connect(); // MQTT
  //connectingMQTT = true;
}

THiNX::THiNX() {
  _thisOTAServer = "http://thinx.cloud:7442";
  //connect(); // MQTT
  //flagSoftwareSerial = true;
}

/* register to HTTP server (no MQTT involved), might result in forced update */
void THiNX::checkin() {
  // HTTP POST here
  conect();
  // if any, save to updateAvailable and update();
  // DEPRECATED: updateAvailable = true; // todo, save update_url
}

void THiNX::end() {
  // exit mqtt to send lwt goodbye
}

/* connect to mqtt and check for update */
void THiNX::connect() {
  // conect to mqtt MQTT set lwt for exit,
  if (true) update();
}

/* local update availability check; returns true if there is internally update available */
bool THiNX::check() {
  // send welcome message to /home/tenant/etc...
  // check for retained message, save to updateAvailable
  if (true) update();
}

void THiNX:mqttCallback(topic, message) {
  update();
}

/* force update (if available) */
bool THiNX:update(char* url) { // use last global url or exit with false/int_error
  // conect to mqtt and set lwt for exit
  if (updateAvailable) {
    // fetch and install
  }

}
