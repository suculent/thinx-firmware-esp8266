# THiNX Lib (ESP)
An Arduino/ESP8266 library to wrap client for OTA updates and RTM (Remote Things Management) based on THiNX platform.

# Useage
## Include

```c
#include "things-lib-esp.h"

```

## Definition
### THiNX Library

The daemon started by library should not require any additional parameters except for optional target host:

* if not defined, defaults to thinx.cloud platform
* either your local `thinx-device-api` instance or [currently non-existent at the time of this writing] `thinx-secure-gateway` which does not exist now, but is planned to provide HTTP to HTTPS bridging from local network to 

```c
THiNX 
THiNX THiNX(&Serial);
```

### SoftwareSerial

```c
THiNX THiNX(10, 11); // RX, TX
```

or

```c
SoftwareSerial mySoftwareSerial(10, 11); //RX, TX
THiNX THiNX(&mySoftwareSerial);
```

Be careful that not all pins support SofwareSerial.

Please check or test whether using pins for SoftwareSerial work or not.

[SoftwareSerial](https://www.arduino.cc/en/Reference/SoftwareSerial)

[ArduinoProducts](https://www.arduino.cc/en/Main/Products)

## Use like Serial

```c
void setup() {
  THiNX.begin(115200);
}

void loop() {
  THiNX.println("Hello world!");
  delay(1000);
}
```

## As HardwareSerial

```c
if (THiNX->isHardwareSerial()) {
  THiNX->println("It is HardwareSerial");
  THiNX->_thisOTAServer->println("Direct print to HardwareSerial");

}
```

## As SoftwareSerial

```c
if (THiNX->isSoftwareSerial()) {
  THiNX->println("It is SoftwareSerial");
  THiNX->thisSoftwareSerial->println("Direct print to SoftwareSerial");
}
```

## As USBAPI Serial_

```c
if (THiNX->isSerial_()) {
  THiNX->println("It is USBAPI Serial_");
  THiNX->thisSerial_->println("Direct print to USBAPI Serial_");
}
```

If you want to use code for multiple platform including any board that does not support Serial_, call `thisSerial_` between `#ifdef __USB_SERIAL_AVAILABLE__` and `#endif`.

```c
#ifdef __USB_SERIAL_AVAILABLE__
  THiNX->thisSerial_->println("Direct print to USBAPI Serial_");
#endif
```

# References
- [HardwareSerial.h](https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/cores/arduino/HardwareSerial.h)
- [SoftwareSerial.h](https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/libraries/SoftwareSerial/src/SoftwareSerial.h)
- [USBAPI.h](https://github.com/arduino/Arduino/blob/2bfe164b9a5835e8cb6e194b928538a9093be333/hardware/arduino/avr/cores/arduino/USBAPI.h)
- [SoftwareSeiral/keywords.txt](https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/libraries/SoftwareSerial/keywords.txt)
