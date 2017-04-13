#ifndef THiNX_H
#define THiNX_H

#include <Arduino.h>
#include <ESP8266HttpUpdate.h>

class THiNX : public Stream {
  public:
  THiNX(char* __thisOTAServer);
  THiNX();
#ifdef __ARDUINO__
  THiNX(Serial_* _thisSerial_);
#endif

  char* _thisOTAServer;

  void register();
  void connect();
  void end();
  virtual bool check();
  virtual bool update(char* url);
  void mqttCallback(topic, message); // when message arrives

  using Print::write;

  private:
    bool connectingMQTT;
    bool updateAvailable;
    /*
  void setAllFlagsFalse();

  bool flagSerial_;
  */

};

#endif
