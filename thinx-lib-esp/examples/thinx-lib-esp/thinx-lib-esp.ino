#include "THiNX.h"

THiNX myHardOrUsbSerial(&Serial);

THiNX mySoftSerial1(10, 11); // RX, TX

SoftwareSerial mySoftwareSerial(8, 9); //RX, TX
THiNX mySoftSerial2(&mySoftwareSerial);

void setup() {
  myHardOrUsbSerial.begin(115200);
  mySoftSerial1.begin(19200);
  mySoftSerial2.begin(9600);
}

void loop() {
  testPrintWithTHiNX(&myHardOrUsbSerial);
  testPrintWithTHiNX(&mySoftSerial1);
  testPrintWithTHiNX(&mySoftSerial2);
  delay(1000);
}

void testPrintWithTHiNX(THiNX* THiNX) {
  THiNX->print("## start some serial at ");
  THiNX->println(millis());
  if (THiNX->isHardwareSerial()) {
    THiNX->println("It is HardwareSerial");
    THiNX->_thisOTAServer->println("Direct print to HardwareSerial");

  } else if (THiNX->isSerial_()) {
    THiNX->println("It is USBAPI Serial_");
#ifdef __USB_SERIAL_AVAILABLE__
    THiNX->thisSerial_->println("Direct print to USBAPI Serial_");
#endif

  } else if (THiNX->isSoftwareSerial()) {
    THiNX->println("It is SoftwareSerial");
    THiNX->thisSoftwareSerial->println("Direct print to SoftwareSerial");
  }
  THiNX->println("## end some serial");
}

