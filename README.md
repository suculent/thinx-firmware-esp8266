# THiNX Lib (ESP)

An Arduino/ESP8266 library to wrap client for OTA updates and RTM (Remote Things Management) based on THiNX platform.

# Success log (library is not finished)

```
*TH: Initializing in 5 seconds...
scandone
no 6RA found, reconnect after 1s
reconnect
f 0, scandone
state: 0 -> 2 (b0)
state: 2 -> 3 (0)
state: 3 -> 5 (10)
add 0
aid 1
cnt
*TH: Imported build-time constants...
*TH: No remote configuration found so far...
EAVWM: Adding parameter
EAVWM: apikey

*TH: AutoConnect with AP Mode (no password):

Soft WDT reset

```

Stack trace:
```
Decoding 24 results
0x402143ac: String::StringIfHelper() const at ?? line ?
0x40203198: delay at ?? line ?
0x402143ac: String::StringIfHelper() const at ?? line ?
0x402031a3: delay at ?? line ?
0x4020f930: ESP8266WiFiSTAClass::waitForConnectResult() at ?? line ?
0x4020f93e: ESP8266WiFiSTAClass::waitForConnectResult() at ?? line ?
0x4020f718: ESP8266WiFiSTAClass::status() at ?? line ?
0x40206dfc: EAVManager::waitForConnectResult() at ?? line ?
0x4020f90c: ESP8266WiFiSTAClass::begin() at ?? line ?
0x4020f8f9: ESP8266WiFiSTAClass::begin() at ?? line ?
0x402143ac: String::StringIfHelper() const at ?? line ?
0x40206f5b: EAVManager::connectWifi(String, String) at ?? line ?
0x40202030: String::changeBuffer(unsigned int) at ?? line ?
0x4020207f: String::reserve(unsigned int) at ?? line ?
0x402020b5: String::copy(char const*, unsigned int) at ?? line ?
0x40202106: String::String(char const*) at ?? line ?
0x40207d97: EAVManager::autoConnect(char const*, char const*) at ?? line ?
0x40201748: Print::println() at ?? line ?
0x4020449e: THiNX::connect() at ?? line ?
0x40206256: THiNX::initWithAPIKey(char const*) at ?? line ?
0x402063cc: THiNX::THiNX(char const*) at ?? line ?
0x40202c40: esp_yield at ?? line ?
0x402031ae: delay at ?? line ?
0x40214e2a: setup at ?? line ?

```
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
