// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include <vector>

#include "basic.h"

#include "../libraries/tinycc/libtcc.h"
#include <dyncall.h>

std::vector<TCCState *> modules;

extern "C" void print_tcc_error(void *b, const char *msg) {
  c_puts(msg); newline();
}

#include "eb_conio.h"
#include "eb_video.h"
#include <stdarg.h>

#define S(n) { #n, (void *)n },
#define R(n, m) { #n, (void *)m },

#ifdef __x86_64__
extern "C" void __va_arg(void);
extern "C" void __va_start(void);
#endif

const struct {
  const char *name;
  const void *addr;
} export_syms[] = {
#include "export_syms.h"
};

#undef S
#undef R

static TCCState *new_tcc() {
  TCCState *tcc = tcc_new();
  if (!tcc)
    return NULL;

  tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY);
  tcc_set_options(tcc, "-nostdlib");

  BString default_include_path =
#ifdef SDL
    BString(getenv("ENGINEBASIC_ROOT")) +
#endif
    BString("/include");
  tcc_add_include_path(tcc, default_include_path.c_str());

  tcc_define_symbol(tcc, "ENGINEBASIC", "1");

  for (auto sym : export_syms) {
    tcc_add_symbol(tcc, sym.name, sym.addr);
  }

  return tcc;
}

void Basic::init_tcc() {
  callvm = dcNewCallVM(4096);
  dcMode(callvm, DC_CALL_C_DEFAULT);

  // add empty module to make default symbols visible to BASIC
  modules.push_back(new_tcc());
}

void *Basic::get_symbol(const char *sym_name) {
  for (auto tcc : modules) {
    void *sym = tcc_get_symbol(tcc, sym_name);
    if (sym)
      return sym;
  }
  return NULL;
}

const char *Basic::get_name(void *addr) {
  for (auto tcc : modules) {
    const char *sym = tcc_get_name(tcc, addr);
    if (sym)
      return sym;
  }
  return NULL;
}

struct Basic::nfc_result Basic::do_nfc(void *sym) {
  static float empty[16] = {};
  struct nfc_result ret;

  dcReset(callvm);

  std::vector<char *> string_list;

  if (!end_of_statement()) {
    do {
      if (is_strexp()) {
        char *arg = strdup(istrexp().c_str());
        if (err)
          goto out;
        string_list.push_back(arg);
        dcArgPointer(callvm, arg);
      } else {
        int arg = (int)iexp();
        if (err)
          goto out;
        dcArgInt(callvm, arg);
      }
    } while (*cip++ == I_COMMA);
    --cip;
  }

  ret.type = 0;
  ret.rint = dcCallInt(callvm, sym);

out:
  for (auto s : string_list) {
    free(s);
  }

  return ret;
}

void Basic::infc() {
  void *sym = *((void **)cip);
  cip += icodes_per_ptr();

  do_nfc(sym);
}

num_t Basic::nnfc() {
  void *sym = *((void **)cip);
  cip += icodes_per_ptr();

  if (checkOpen())
    return 0;

  struct nfc_result ret = do_nfc(sym);

  if (checkClose())
    return 0;

  return ret.rint;
}
