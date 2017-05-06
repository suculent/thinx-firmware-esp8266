#ifndef THiNX_H
#define THiNX_H

#include <Arduino.h>
#include <ESP8266HttpUpdate.h>
#include <ESP8266WifiClient.h>

class THiNX : public Stream {
  public:
  THiNX(char* __thisOTAServer);
  THiNX();

  char* _thisOTAServer;

  void checkin();
  void update();
  void end();

  private:
    bool connectingMQTT;
    bool updateAvailable;
    
    virtual bool check();
    virtual bool update(char* url);
    void mqttCallback(topic, message); // when message arrivee
};

#endif
