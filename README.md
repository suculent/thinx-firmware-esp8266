# THiNX Lib (ESP)

An Arduino/ESP8266 library to wrap client for OTA updates and RTM (Remote Things Management) based on THiNX platform.

# Useage
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
#include <THiNXLib.h>

THiNX thx;

void setup() {
  Serial.begin(115200);

#ifdef __DEBUG__
  while (!Serial);
#else
  delay(500);
#endif

  thx = THiNX("71679ca646c63d234e957e37e4f4069bf4eed14afca4569a0c74abf503076732"); // THINX_API_KEY
}

void loop()
{
  delay(10000);
  thx.loop(); // registers, checks MQTT status, reconnects, updates, etc.
}

```
