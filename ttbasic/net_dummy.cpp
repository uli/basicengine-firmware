#include "ttconfig.h"

#ifndef HAVE_NETWORK

#include "basic.h"

void Basic::inet() {
  err = ERR_NOT_SUPPORTED;
}
num_t Basic::nconnect() {
  err = ERR_NOT_SUPPORTED;
  return 0;
}

void Basic::iwget() {
  err = ERR_NOT_SUPPORTED;
}

#endif

#if !defined(HAVE_NETWORK) || !defined(HAVE_TFTP)

void Basic::itftpd() {
  err = ERR_NOT_SUPPORTED;
}

void Basic::itftpget() {
  err = ERR_NOT_SUPPORTED;
}

void Basic::itftpput() {
  err = ERR_NOT_SUPPORTED;
}

#endif
