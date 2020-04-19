#ifndef _ESP_IR_CONTROLLER_H
#define _ESP_IR_CONTROLLER_H

#include "Arduino.h"

#if defined(ESP8266) || defined(ESP32)

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

class EspIRController {
public:
  EspIRController(byte sendPin=D1, byte recvPin=D4, bool recvEnabled=false);  // IRBlaster defaults
  ~EspIRController();

  void setup();
  void loop();
  bool send(String data, String type);
  void enableRecv(bool enabled) { mRecvEnabled = enabled; };

private:
  byte mSendPin, mRecvPin;
  bool mRecvEnabled;
};

extern EspIRController espIRController;

#endif  // ESP8266 || ESP32

#endif  // _ESP_IR_CONTROLLER_H
