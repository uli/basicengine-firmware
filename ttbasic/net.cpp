// SPDX-License-Identifier: MIT
/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2017-2019 Ulrich Hecht.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#include "ttconfig.h"
#if defined(HAVE_NETWORK)

#include "basic.h"
#include "net.h"

void E_NETWORK(const BString &msg) {
  static BString net_err;
  err = ERR_NETWORK;
  net_err = msg;
  err_expected = net_err.c_str();
}

#if defined(ESP8266) || defined(ESP32)

#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
typedef ESP8266WiFiMulti WiFiMulti;
#else
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#endif

WiFiMulti wm;
static HTTPClient *http = NULL;
static WiFiClient *stream = NULL;

void Basic::isetap() {
  BString ssid = istrexp();
  if (err)
    return;
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return;
  }
  BString password = istrexp();

  wm.addAP(ssid.c_str(), password.c_str());
}

void iconnect() {
  wm.run();
}

num_t Basic::nconnect() {
  if (checkOpen() || checkClose())
    return 0;
  return wm.run();
}

static int open_url(BString &url) {
  if (http) {
    http->end();
    delete http;
  }
  http = new HTTPClient;
  if (!http) {
    err = ERR_OOM;  // I guess...
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

void Basic::inetopen() {
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

void Basic::inetclose() {
  if (!http) {
    E_NETWORK(PSTR("no connection"));
    return;
  }
  http->end();
  delete http;
  http = NULL;
  stream = NULL;
}

BString Basic::snetinput() {
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

BString Basic::snetget() {
  BString rx;
  if (checkOpen())
    return rx;
  BString url = istrexp();
  if (err || checkClose())
    return rx;
  if (wm.run() != WL_CONNECTED) {
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

void Basic::iwget() {
  BString url = istrexp();
  BString file;
  if (*cip == I_TO) {
    ++cip;
    file = getParamFname();
  } else {
    int sl = url.lastIndexOf('/');
    if (sl < 0)
      file = F("index.html");
    else
      file = url.substring(sl + 1);
  }
  if (err)
    return;
  if (wm.run() != WL_CONNECTED) {
    E_NETWORK(F("not connected"));
    return;
  }
  int httpCode = open_url(url);
  if (err)
    return;
  FILE *f = fopen(file.c_str(), "w");
  if (!f) {
    err = ERR_FILE_OPEN;
    inetclose();
    return;
  }
  int total = 0;
  if (httpCode == HTTP_CODE_OK) {
    WiFiClient *stream = http->getStreamPtr();
    char buf[128];
    while (http->connected()) {
      int av = stream->available();
      if (!av) {
        delay(1);
      } else {
        int count = stream->readBytes(buf, av > 128 ? 128 : av);
        fwrite(buf, 1, count, f);
        total += count;
      }
    }
  } else {
    E_NETWORK(BString(F("E")) + BString(httpCode));
  }
  fclose(f);
  inetclose();
  retval[0] = total;
}

void Basic::inet() {
  switch (*cip++) {
  case I_CONFIG:  isetap();    break;
  case I_CONNECT: iconnect();  break;
  case I_OPEN:    inetopen();  break;
  case I_CLOSE:   inetclose(); break;
  default:        E_ERR(SYNTAX, _("expected network command")); break;
  }
}

#endif	// ESP8266 || ESP32

#endif	// HAVE_NETWORK

