// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include <string>

#include "eb_api.h"
#include "eb_sys.h"
#include "basic.h"
#include "basic_native.h"

EBAPI void eb_wait(unsigned int ms) {
  unsigned end = ms + millis();
  while (millis() < end) {
    if (eb_process_events_wait())
      break;
  }
}

EBAPI unsigned int eb_tick(void) {
  return millis();
}

EBAPI unsigned int eb_utick(void) {
  return micros();
}

EBAPI void eb_udelay(unsigned int us) {
  return delayMicroseconds(us);
}

// NB: process_events() is directly exported to native modules as
// eb_process_events(), so changing this will not necessarily have the
// desired effect.
EBAPI void eb_process_events(void) {
  process_events();
}

EBAPI int eb_process_events_check(void) {
  process_events();
  return process_hotkeys(sc0.peekKey());
}

EBAPI int eb_process_events_wait(void) {
  process_events();

  if (process_hotkeys(sc0.peekKey()))
    return 1;

  yield();
  return 0;
}

#ifndef __linux__
EBAPI void eb_set_cpu_speed(int percent) {
#ifdef H3
  int factor;

  if (percent < 0)
    factor = SYS_CPU_MULTIPLIER_DEFAULT;
  else
    factor = SYS_CPU_MULTIPLIER_MAX * percent / 100;

  sys_set_cpu_multiplier(factor);
#else
  // unsupported, ignore silently
#endif
}
#endif

#include <miniz.h>
#include "eb_file.h"

EBAPI int eb_install_module(const char *filename) {
  if (!eb_file_exists(filename)) {
    err = ERR_FILE_OPEN;
    return -1;
  }

  char cwd[FILENAME_MAX];
  if (!getcwd(cwd, FILENAME_MAX)) {
    err = ERR_SYS;
    return -1;
  }

  BString absfile = BString(cwd) + BString("/") + BString(filename);

  BString moddir = BString(getenv("HOME")) + BString("/sys/modules");
  if (chdir(moddir.c_str())) {
    if (mkdir(moddir.c_str()
#ifndef _WIN32
                            , 0755
#endif
                                  )) {
      err = ERR_IO;
      err_expected = _("cannot create module directory");
      return -1;
    }
    if (chdir(moddir.c_str())) {
      err = ERR_SYS;
      return -1;
    }
  }

  eb_unzip(absfile.c_str(), 0);

  if (chdir(cwd)) {
    err = ERR_SYS;
    return -1;
  }

  return 0;
}

#include "eb_native.h"

EBAPI int eb_load_module(const char *name) {
  char cwd[FILENAME_MAX];
  if (!getcwd(cwd, FILENAME_MAX)) {
    err = ERR_SYS;
    return -1;
  }

  BString moddir;
  BString modname;
  BString namestr(name);

  if (namestr.indexOf('/') == -1) {
    moddir = BString(getenv("HOME")) + BString("/sys/modules/") + namestr;
    modname = namestr;
  } else {
    moddir = namestr;
    modname = namestr.substring(namestr.lastIndexOf('/') + 1, namestr.length());
  }

  if (chdir(moddir.c_str()))
    goto not_found;

  if (eb_file_exists("loader.bas")) {
    void *bc = eb_new_basic_context();
    if (!bc) {
      err = ERR_OOM;
      return -1;
    }
    eb_exec_basic(bc, "loader.bas");
    eb_delete_basic_context(bc);
  } else {
    BString objname = modname + BString("_") + BString(getenv("HOSTTYPE")) + BString(".o");
    if (!eb_file_exists(objname.c_str()))
      goto not_found;

    TCCState *tcc = eb_tcc_new(TCC_OUTPUT_MEMORY);
    tcc_set_error_func(tcc, NULL, print_tcc_error);

    eb_tcc_initialize_symbols(tcc);

    if (tcc_add_file(tcc, objname.c_str()) < 0 ||
        eb_tcc_link(tcc, name, TCC_OUTPUT_MEMORY) < 0) {
      tcc_delete(tcc);
      err = ERR_COMPILE;
      return -1;
    }

    void (*initcall)(void) = (void(*)())Basic::get_symbol("__initcall");
    if (initcall)
      initcall();
  }

  if (chdir(cwd)) {
    if (!err)
      err = ERR_SYS;
    return -1;
  }
  return 0;

not_found:
  err = ERR_FILE_OPEN;
  err_expected = _("module not found");
  return -1;
}

EBAPI int eb_module_count(void) {
  return modules.size();
}

EBAPI const char *eb_module_name(int index) {
  if (index < modules.size())
    return modules[index].name.c_str();
  else
    return NULL;
}
