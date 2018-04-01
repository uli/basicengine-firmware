#include "basic.h"
#include "net.h"

#ifndef ESP8266_NOWIFI

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

ESP8266WiFiMulti WiFiMulti;

void isetap() {
  BString ssid = istrexp();
  if (err)
    return;
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return;
  }
  BString password = istrexp();
  
  WiFiMulti.addAP(ssid.c_str(), password.c_str());
}

void iconnect() {
  WiFiMulti.run();
}

void inet() {
  if (*cip == I_CONFIG) {
    ++cip;
    isetap();
  } else if (*cip == I_CONNECT) {
    ++cip;
    iconnect();
  }
  E_ERR(SYNTAX, "network command");
}

num_t nconnect() {
  if (checkOpen() || checkClose())
    return 0;
  return WiFiMulti.run();
}

BString swget() {
  BString rx;
  if (checkOpen()) return rx;
  BString url = istrexp();
  if (err || checkClose())
    return rx;
  if (WiFiMulti.run() != WL_CONNECTED) {
    return BString(F("ENC"));
  }
  HTTPClient http;
  http.begin(url.c_str());
  int httpCode = http.GET();
  if (httpCode < 0) {
    http.end();
    return BString(F("EGT ")) + http.errorToString(httpCode).c_str();
  }
  if (httpCode == HTTP_CODE_OK) {
    rx = http.getString().c_str();
  } else {
    rx = BString(F("E")) + BString(httpCode);
  }
  http.end();
  return rx;
}

#else

void inet() {
  err = ERR_NOT_SUPPORTED;
}
num_t nconnect() {
  err = ERR_NOT_SUPPORTED;
  return 0;
}
BString swget() {
  err = ERR_NOT_SUPPORTED;
  return BString(F(""));
}
#endif
