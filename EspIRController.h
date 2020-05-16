#ifndef _ESP_IR_CONTROLLER_H
#define _ESP_IR_CONTROLLER_H

#include "Arduino.h"

#if defined(ESP8266) || defined(ESP32)

#define RemoteType(id)  (String)#id

#include "FS.h"

#ifdef ESP32
  #include "SPIFFS.h"
#endif

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "EspWifi.h"

class EspIRController {
public:
  EspIRController(byte sendPin=5, byte recvPin=2);  // IRBlaster defaults
  ~EspIRController();

  void setup();
  void loop();
  bool send(String data, String type, uint16_t repeat=1, uint16_t repeatDelay=10);

protected:
  class EspIRControllerRequestHandler : public EspWiFiRequestHandler {
  public:
    bool canHandle(HTTPMethod method, String uri);
    bool canUpload(String uri);
  #ifdef ESP8266
    bool handle(ESP8266WebServer& server, HTTPMethod method, String uri);
    void upload(ESP8266WebServer& server, String uri, HTTPUpload& upload);
  protected:
    bool httpHandleSendIRCmd(ESP8266WebServer& server);
    bool httpHandleRecvIRCmd(ESP8266WebServer& server);
  #endif
  #ifdef ESP32
    bool handle(WebServer& server, HTTPMethod method, String uri);
    void upload(WebServer& server, String uri, HTTPUpload& upload);
  protected:
    bool httpHandleSendIRCmd(WebServer& server);
    bool httpHandleRecvIRCmd(WebServer& server);
  #endif
  
  protected:
  #ifdef ESP8266
    bool canHandle(ESP8266WebServer& server) override;
  #endif
  #ifdef ESP32
    bool canHandle(WebServer& server) { return false; } override;
  #endif
    String menuHtml() override;
    uint8_t menuIdentifiers() override;
    String menuIdentifiers(uint8_t identifier) override;
    String menuIdentifierIR() { return "ir"; };
  
  private:
    File uploadFile;
    String mLastLoadedRemote = "";
  
    String getRemoteCssUri() { return "/static/remote.css"; };
    String getRemoteJsUri() { return "/static/remote.js"; };
    String getRemoteDirUri() { return "/remote/"; };
    String getRemoteDevUri(String device) { return getRemoteDirUri() + device + ".js"; };
    String getRemoteFromUri(String uri) { String remote = uri.substring(getRemoteDirUri().length()); return remote.substring(0, remote.indexOf(".js")); };
    String getUploadUri() { return getRemoteDirUri() + "upload_" + EspWiFi::getChipID(); };
    String getSendIRUri() { return "/sendIRCmd"; };
    String getRecvIRUri() { return "/recvIRCmd"; };
    String getIRFormHtml();
    String getIRForm(String action, String remote="");

  } mEspIRControllerRequestHandler;

private:
  byte mSendPin, mRecvPin;

  String remoteType(decode_type_t type);
  decode_type_t remoteType(String type);
  String Uint64toString(uint64_t input, uint8_t base);

  IRrecv *startRecv(uint16_t bufSize=100U, uint16_t timeout=15U) { if (mIRRecv==NULL)mIRRecv = new IRrecv(mRecvPin, bufSize, timeout); mIRRecv->enableIRIn(); return mIRRecv; };
  void stopRecv() { if (mIRRecv!=NULL) mIRRecv->disableIRIn(); };

  IRsend *mIRSend = NULL;
  IRrecv *mIRRecv = NULL;
};

extern EspIRController espIRController;

#endif  // ESP8266 || ESP32

#endif  // _ESP_IR_CONTROLLER_H
