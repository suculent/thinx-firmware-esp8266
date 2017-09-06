# THiNX Lib (ESP)

An Arduino/ESP8266 library to wrap client for OTA updates and RTM (Remote Things Management) based on THiNX platform.

# Usage
## Include

```c
#include "THiNXLib.h"
```

## Definition
### THiNX Library

The singleton class started by library should not require any additional parameters except for optional API Key.
Connects to WiFI and reports to main THiNX server; otherwise starts WiFI in AP mode (AP-THiNX with password PASSWORD by default)
and awaits optionally new API Key (security hole? FIXME: In case the API Key is saved (and validated) do not allow change from AP mode!!!).

* if not defined, defaults to thinx.cloud platform
* TODO: either your local `thinx-device-api` instance or [currently non-existent at the time of this writing] `thinx-secure-gateway` which does not exist now, but is planned to provide HTTP to HTTPS bridging from local network to

```c
#include "Arduino.h"
#include "FS.h"
#include <THiNXLib.h>
#include "Settings.h" // ssid, pass and API Key

THiNX thx;

bool once = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.setDebugOutput(true);

  wdt_disable(); // causes wdt reset after 8 seconds!
  wdt_enable(65535); // must be called from wdt_disable() state!
  delay(3000);
  Serial.print("THiNXLib v");
  Serial.println(VERSION);

  // Force override WiFi before attempting to connect
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin();

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
  }
}

void loop()
{
    if (once) {
      thx.loop(); // calling the loop is important to let MQTT work in background
    } else {
      once = true;
      thx = THiNX(apikey);
      if (WiFi.status() == WL_CONNECTED) {
        thx.connected = true; // force checkin
      }
    }
    Serial.println(millis());
    delay(100);
}
```
