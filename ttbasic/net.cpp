#include "basic.h"
#include "net.h"

#ifndef ESP8266_NOWIFI

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

ESP8266WiFiMulti WiFiMulti;
static HTTPClient *http = NULL;
static WiFiClient *stream = NULL;

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

num_t nconnect() {
  if (checkOpen() || checkClose())
    return 0;
  return WiFiMulti.run();
}

static int open_url(BString &url) {
  if (http) {
    http->end();
    delete http;
  }
  http = new HTTPClient;
  if (!http) {
    err = ERR_OOM;	// I guess...
    return -1;
  }
  http->begin(url.c_str());
  int httpCode = http->GET();
  if (httpCode < 0) {
    http->end();
    E_NETWORK(http->errorToString(httpCode).c_str());
    delete http;
    http = NULL;
  }
  return httpCode;
}

void inetopen() {
  BString url = istrexp();
  BString uh(F("http://"));
  BString uhs(F("https://"));
  if (!url.startsWith(uh) && !url.startsWith(uhs)) {
    E_ERR(VALUE, "unsupported URL");
    return;
  }
  retval[0] = open_url(url);
  stream = http->getStreamPtr();
}

void inetclose() {
  if (!http) {
    E_NETWORK(PSTR("no connection"));
    return;
  }
  http->end();
  delete http;
  http = NULL;
  stream = NULL;
}

BString snetinput() {
  BString rx;
  size_t count = getparam();
  if (err)
    return rx;
  if (!http || !stream) {
    E_NETWORK(PSTR("no connection"));
    return rx;
  }
  size_t avail = stream->available();
  count = avail < count ? avail : count;
  if (!rx.reserve(count)) {
    err = ERR_OOM;
    return rx;
  }
  count = stream->readBytes(rx.begin(), count);
  rx.resetLength(count);
  return rx;
}

BString snetget() {
  BString rx;
  if (checkOpen()) return rx;
  BString url = istrexp();
  if (err || checkClose())
    return rx;
  if (WiFiMulti.run() != WL_CONNECTED) {
    return BString(F("ENC"));
  }
  int httpCode = open_url(url);
  if (err)
    return rx;
  if (httpCode == HTTP_CODE_OK)
    rx = http->getString().c_str();
  else
    rx = BString(F("E")) + BString(httpCode);
  inetclose();
  return rx;
}

void inet() {
  switch (*cip++) {
  case I_CONFIG:
    isetap();
    break;
  case I_CONNECT:
    iconnect();
    break;
  case I_OPEN:
    inetopen();
    break;
  case I_CLOSE:
    inetclose();
    break;
  default:
    E_ERR(SYNTAX, "network command");
    break;
  }
}

#else

void inet() {
  err = ERR_NOT_SUPPORTED;
}
num_t nconnect() {
  err = ERR_NOT_SUPPORTED;
  return 0;
}
#endif
