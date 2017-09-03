# THiNX Lib (ESP)

An Arduino/ESP8266 library to wrap client for OTA updates and RTM (Remote Things Management) based on THiNX platform.

# Success log (library is not finished)

Currently crashes somewhere in the middle of senddata(String) ; should be refactored to const char *

```
*TH: WiFi connected, checking in...
*TH: Starting API checkin...
{"registration":{"mac":"5CCF7FF0949A","firmware":"thinx-firmware-arduinoc-2.0.34:2017-08-28","version":"2.0.34","commit":"14ee4171ac64e8e641e2cecf2000aaac9959113b","owner":"cedc
16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12","alias":"robotdyn-esp8266-wifi-node","platform":"platformio"}}
*THiNXLib::senddata(): connected...*THiNXLib::senddata(): with api key...*THiNXLib::senddata(): waiting for response...*THiNXLib::senddata(): parsing payload...*TH: Parsing resp
onse: '{"registration":{"success":true,"owner":"cedc16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12","alias":"robotdyn-esp8266-wifi-node","udid":"816067f0-90ec-11e7
-bf7f-3d24207fb6af","status":"OK"}}'
commit:
version:
*TH: building device info:
*TH: thinx_alias: robotdyn-esp8266-wifi-node
*TH: thinx_owner: cedc16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12
*TH: thinx_api_key: 71679ca646c63d234e957e37e4f4069bf4eed14afca4569a0c74abf503076732
*TH: thinx_udid: 816067f0-90ec-11e7-bf7f-3d24207fb6af
*TH: available_update_url:

*TH: saving configuration to EEPROM...{"alias":"robotdyn-esp8266-wifi-node","owner":"cedc16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12","apikey":"71679ca646c63d23
4e957e37e4f4069bf4eed14afca4569a0c74abf503076732","udid":"816067f0-90ec-11e7-bf7f-3d24207fb6af","update":""}
*TH: EEPROM data committed...
```

Stack trace:
```
0x4020b252: THiNX::senddata(String) at ??:?
0x4020b252: THiNX::senddata(String) at ??:?
0x4020b24c: THiNX::senddata(String) at ??:?
0x40201848: Print::println() at ??:?
0x40201848: Print::println() at ??:?
0x4020b41f: std::_Function_handler<void (MQTT::Publish const&), THiNX::start_mqtt()::{lambda(MQTT::Publish const&)#1}>::_M_invoke(std::_Any_data const&, MQTT::Publish const&) at THiNXLib.cpp:?
0x40201894: Print::println(char const*) at ??:?
0x4020b8a4: MQTT::Connect::~Connect() at ??:?
0x4020fd6e: loop at ??:?

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
