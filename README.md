# THiNX Lib (ESP)

An Arduino/ESP8266 library to wrap client for OTA updates and RTM (Remote Things Management) based on THiNX platform.

# Success log (library is not finished)

```
*TH: Initializing...
*TH: Imported build-time constants...
*TH: Restoring device info...
*TH: No remote configuration found so far...
EAVWM: Adding parameter
EAVWM: apikey
*TH: AutoConnect with AP Mode (no password):
EAVWM:
EAVWM: AutoConnect
EAVWM: Connecting as wifi client...Fatal exception 3(LoadStoreErrorCause):
epc1=0x40203839, epc2=0x00000000, epc3=0x00000000, excvaddr=0x402141bd, depc=0x00000000
Exception (3):
epc1=0x40203839 epc2=0x00000000 epc3=0x00000000 excvaddr=0x402141bd depc=0x00000000
```

Stack trace:
```
Exception 3: LoadStoreError: Processor internal physical address or data error during load or store
Decoding 26 results
0x40203839: uart_write_char at ?? line ?
0x402141bd: Print::write(unsigned char const*, unsigned int) at ?? line ?
0x40203839: uart_write_char at ?? line ?
0x402141bd: Print::write(unsigned char const*, unsigned int) at ?? line ?
0x402141b4: Print::write(unsigned char const*, unsigned int) at ?? line ?
0x4020164d: Print::write(char const*) at ?? line ?
0x4024eccb: chip_v6_unset_chanfreq at ?? line ?
0x4020164d: Print::write(char const*) at ?? line ?
0x40201748: Print::println() at ?? line ?
0x4024eccb: chip_v6_unset_chanfreq at ?? line ?
0x4020176c: Print::println(__FlashStringHelper const*) at ?? line ?
0x4024eccb: chip_v6_unset_chanfreq at ?? line ?
0x4020656e: _ZN10EAVManager8DEBUG_WMIPK19__FlashStringHelperEEvT_$isra$8 at EAVManager.cpp line ?
0x40206f1d: EAVManager::connectWifi(String, String) at ?? line ?
0x40202030: String::changeBuffer(unsigned int) at ?? line ?
0x4020207f: String::reserve(unsigned int) at ?? line ?
0x40202106: String::String(char const*) at ?? line ?
0x4024ed1c: chip_v6_unset_chanfreq at ?? line ?
0x40207e33: EAVManager::autoConnect(char const*, char const*) at ?? line ?
0x40201748: Print::println() at ?? line ?
0x40204493: THiNX::connect() at ?? line ?
0x4020624f: THiNX::initWithAPIKey(char const*) at ?? line ?
0x402063d8: THiNX::THiNX(char const*) at ?? line ?
0x40201748: Print::println() at ?? line ?
0x402031ae: delay at ?? line ?
0x40214eab: setup at ?? line ?
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
