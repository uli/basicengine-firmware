// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

#ifdef JAILHOUSE

#include "ttconfig.h"
#include "basic.h"
#include "basic_native.h"
#include "eb_file.h"
#include "net.h"

#include <errno.h>
#include <list>
#include <stdlib.h>
#include <sys/wait.h>

static int run_command(const char *cmd)
{
  int ret = system(cmd);
  if (ret == -1) {
    BString error(_("System error: "));
    error += strerror(errno);
    E_NETWORK(error);
  }
  ret = WEXITSTATUS(ret);
  return ret;
}

static void iconnect() {
  // XXX: select interface
  if (run_command("udhcpc eth0") != 0)
    E_NETWORK(_("could not start DHCP client"));
}

static void idisconnect() {
  // XXX: select interface
  system("killall udhcpc");
}

void Basic::iwget() {
  BString url = istrexp();
  BString file;
  if (*cip == I_TO) {
    ++cip;
    file = getParamFname();
  } else {
    int sl = url.lastIndexOf('/');
    if (sl < 0 || sl + 1 == url.length())
      file = F("index.html");
    else
      file = url.substring(sl + 1);
  }
  if (err)
    return;

  // XXX: check if link is up

  std::list<BString> cmd;
  cmd.push_back(BString("wget"));
  cmd.push_back(BString("-O"));
  cmd.push_back(file);
  cmd.push_back(url);

  retval[1] = shell_list(cmd);
  retval[0] = eb_file_size(file.c_str());
}

void Basic::inet() {
  switch (*cip++) {
  case I_CONNECT:
  case I_UP:
    iconnect();
    break;
  case I_DOWN:
    idisconnect();
    break;
  default:
    E_ERR(SYNTAX, _("expected network command"));
    break;
  }
}

BString Basic::snetinput() {
  // XXX: unimplemented
  BString rx;
  size_t count = getparam();
  return rx;
}

BString Basic::snetget() {
  // unimplemented
  BString rx;
  if (checkOpen())
    return rx;
  BString url = istrexp();
  if (err || checkClose())
    return rx;
  return rx;
}

num_t Basic::nconnect() {
  // XXX: unimplemented
  return 0;
}

#endif
