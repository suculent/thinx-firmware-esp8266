/* OTA enabled firmware for Wemos D1 (ESP 8266, Arduino) */

#include <Arduino.h>
#include <ESP8266httpUpdate.h>

// Requires connectivity, that should be served through WiFiManager
char ssid[] = "<ssid>";     //  your network SSID (name)
char pass[] = "<password>";  // your network password
int status = WL_IDLE_STATUS;

void setup() {
  Serial.begin(115200);
  while (!Serial);

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

  Serial.println("[update] WiFi connected, trying direct update...");

  t_httpUpdate_return ret = ESPhttpUpdate.update("207.154.200.217", 80, "/bin/5ccf7fee90e0.bin");

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

    ret = ESPhttpUpdate.update("207.154.200.217", 80, "ota.php", "5ccf7fee90e0");

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

void loop() {
  //
}
