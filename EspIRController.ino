#if defined(ESP8266) || defined(ESP32)

#include "EspIRController.h"

EspIRController::EspIRController(byte sendPin, byte recvPin, bool recvEnabled) {
  mSendPin = sendPin;
  mRecvPin = recvPin;
  mRecvEnabled = recvEnabled;
}

EspIRController::~EspIRController() {
  
}

void EspIRController::setup() {
  
}

void EspIRController::loop() {
  
}

bool EspIRController::send(String data, String type) {
  return false;
}

#endif  // ESP8266 || ESP32

