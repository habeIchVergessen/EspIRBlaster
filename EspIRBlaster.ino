#include "Arduino.h"

#define PROGNAME "EspIRBlaster"
#define PROGVERS "0.1"
#define PROGBUILD String(__DATE__) + " " + String(__TIME__)


#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include "ESP8266WebServer.h"
#endif

#ifdef ESP32
  #include <WiFi.h>
  #include "WebServer.h"
  #include "Update.h"
#endif

#include "EspConfig.h"
#include "EspDebug.h"
#include "EspWifi.h"
#include "EspIRController.h"

// KeyValueProtocol with full format keys
//#define KVP_LONG_KEY_FORMAT 1
#include "KVPSensors.h"

bool httpRequestProcessed     = false;
bool optionsChanged           = false;

bool sendKeyValueProtocol = true;

byte tPin = 14; // D5 - MCP23017 interrupt

bool pirInt = false, touchInt = false;

// global objects
EspConfig espConfig(PROGNAME);
EspWiFi espWiFi;
EspDebug espDebug;

EspIRController espIRController;

unsigned long last = 0;

void setup() {
  Serial.begin(115200);
  yield();

  espDebug.enableSerialOutput();

  DBG_PRINTLN("\n\n");
  DBG_PRINTLN("Test");

  espConfig.setup();
  
  EspWiFi::setup();

  espDebug.begin();
  espDebug.registerInputCallback(handleInputStream);
}

void loop() {
  // handle input
  handleInputStream(&Serial);

  // handle wifi
  EspWiFi::loop();

  // tools
  loopEspTools();

  // send debug data
  espDebug.loop();
}

void handleInput(char r, bool hasValue, unsigned long value, bool hasValue2, unsigned long value2) {
  switch (r) {
    case 'r':
      DBG_PRINTLN("reset: " + uptime());
#ifdef ESP8266
      ESP.reset();
#endif
#ifdef ESP32
      ESP.restart();
#endif
      break;
    case 'u':
      DBG_PRINTLN("uptime: " + uptime());
      printHeapFree();
      break;
    case 'v':
      // Version info
      handleCommandV();
      print_config();
      break;
    default:
      handleCommandV();
      DBG_PRINTLN("uptime: " + uptime());
/*
#ifndef NOHELP
      Help::Show();
#endif
*/      break;
    }
}

void handleCommandV() {
  DBG_PRINT(F("["));
  DBG_PRINT(PROGNAME);
  DBG_PRINT(F("."));
  DBG_PRINT(PROGVERS);

  DBG_PRINT(F("] "));
  DBG_PRINT("compiled at " + PROGBUILD + " ");
}

void handleInputStream(Stream *input) {
  if (input->available() <= 0)
    return;

  static long value, value2;
  bool hasValue, hasValue2;
  char r = input->read();

  // reset variables
  value = 0; hasValue = false;
  value2 = 0; hasValue2 = false;
  
  byte sign = 0;
  // char is a number
  if ((r >= '0' && r <= '9') || r == '-'){
    byte delays = 2;
    while ((r >= '0' && r <= '9') || r == ',' || r == '-') {
      if (r == '-') {
        sign = 1;
      } else {
        // check value separator
        if (r == ',') {
          if (!hasValue || hasValue2) {
            print_warning(2, "format");
            return;
          }
          
          hasValue2 = true;
          if (sign == 0) {
            value = value * -1;
            sign = 0;
          }
        } else {
          if (!hasValue || !hasValue2) {
            value = value * 10 + (r - '0');
            hasValue = true;
          } else {
            value2 = value2 * 10 + (r - '0');
            hasValue2 = true;
          }
        }
      }
            
      // wait a little bit for more input
      while (input->available() == 0 && delays > 0) {
        delay(20);
        delays--;
      }

      // more input available
      if (delays == 0 && input->available() == 0) {
        return;
      }

      r = input->read();
    }
  }

  // Vorzeichen
  if (sign == 1) {
    if (hasValue && !hasValue2)
      value = value * -1;
    if (hasValue && hasValue2)
      value2 = value2 * -1;
  }

  handleInput(r, hasValue, value, hasValue2, value2);
}

String getDictionary() {
  String result = "";

#ifndef KVP_LONG_KEY_FORMAT
//  while (IWEAS_v40::hasNext() != NULL)
//    result += DictionaryValuePort(Load, IWEAS_v40::getNextInstance()->getName());

  result += DictionaryValue(Voltage);
#endif

  return result;
}

void printHeapFree() {
#ifdef _DEBUG_HEAP
  DBG_PRINTLN((String)F("heap: ") + (String)(ESP.getFreeHeap()));
#endif
}

// helper
void print_config() {
  String blank = F(" ");
  
  DBG_PRINT(F("config:"));
  DBG_PRINTLN();
}

void print_warning(byte type, String msg) {
  return;
  DBG_PRINT(F("\nwarning: "));
  if (type == 1)
    DBG_PRINT(F("skipped incomplete command "));
  if (type == 2)
    DBG_PRINT(F("wrong parameter "));
  if (type == 3)
    DBG_PRINT(F("failed: "));
  DBG_PRINTLN(msg);
}

