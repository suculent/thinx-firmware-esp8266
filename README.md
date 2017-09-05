# THiNX Lib (ESP)

An Arduino/ESP8266 library to wrap client for OTA updates and RTM (Remote Things Management) based on THiNX platform.

# Success log (library is not finished)

Currently crashes somewhere after subscribing to MQTT.

```
*TH: WiFi connected, checking in...
*TH: Starting API checkin...
{"registration":{"mac":"5CCF7FC3AF7C","firmware":"thinx-firmware-arduinoc-2.0.47:2017-08-28","version":"2.0.47","commit":"8746e33a99b24eb2488498fa26e19b19ba79a606","owner":"cedc
16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12","alias":"wemos-d1-mini-2.1.0","platform":"platformio"}}
*THiNXLib::senddata(): with api key...
*THiNXLib::senddata(): waiting for response...*THiNXLib::senddata(): parsing payload...*TH: Parsing response: '{"registration":{"success":true,"owner":"cedc16bb6bb06daaa3ff6d306
66d91aacd6e3efbf9abbc151b4dcade59af7c12","alias":"wemos-d1-mini-2.1.0","udid":"3996e660-9227-11e7-97f8-4114a6c898df","status":"OK"}}'
commit:
version:
*TH: saving configuration to EEPROM...
{"alias":"wemos-d1-mini-2.1.0","owner":"cedc16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12","update":"","apikey":"71679ca646c63d234e957e37e4f4069bf4eed14afca4569a0
c74abf503076732","udid":"3996e660-9227-11e7-97f8-4114a6c898df"}

*TH: EEPROM data committed...
12649

*TH: WiFi connected, starting MQTT...
*TH: UDID: 3996e660-9227-11e7-97f8-4114a6c898df
*TH: Contacting MQTT server rtm.thinx.cloud
*TH: Starting client on port 1883
*TH: Connecting to MQTT...
*TH: AK: 71679ca646c63d234e957e37e4f4069bf4eed14afca4569a0c74abf503076732
*TH: DCH: /cedc16bb6bb06daaa3ff6d30666d91aacd6e3efbf9abbc151b4dcade59af7c12/3996e660-9227-11e7-97f8-4114a6c898df
12837
*TH: MQTT Publishing device status: *TH: MQTT Subscribing device channel: �␁
*TH: DCH �␁ successfully subscribed.

Fatal exception 9(LoadStoreAlignmentCause):
epc1=0x402015c6, epc2=0x00000000, epc3=0x00000000, excvaddr=0x4022e01a, depc=0x00000000
```

Stack trace:
```
Exception (9):
epc1=0x402015c6 epc2=0x00000000 epc3=0x00000000 excvaddr=0x4022e01a depc=0x00000000

ctx: sys
sp: 3fff07b0 end: 3fffffb0 offset: 01a0

>>>stack>>>
3fff0950:  4010001e 00000000 00000000 00000000
3fff0960:  3fff099b 00000000 00000000 00000000
3fff0970:  00000000 00000000 3fff099a 402017b5
3fff0980:  00000000 00000000 00000000 00000000
3fff0990:  3fff2bd4 3fff344c 31ff2b5c 38303731
3fff09a0:  40203000 00000001 00000001 3fff05f4
3fff09b0:  00000009 3fff0430 3ffef930 40206be5
3fff09c0:  3ffee560 3fff1670 3fff0600 3fff05f4
3fff09d0:  3fffdad0 00000000 3fff0540 40201866
3fff09e0:  4020301e 00000064 00000064 402018c1
3fff09f0:  3fffdad0 00000000 3ffef930 4020fa06

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
