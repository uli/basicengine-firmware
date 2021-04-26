// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include <vector>

#include "basic.h"
#include "basic_native.h"

#include "../libraries/tinycc/libtcc.h"
#include <dyncall.h>

std::vector<TCCState *> modules;

extern "C" void print_tcc_error(void *b, const char *msg) {
  c_puts(msg); newline();
}


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
  tcc_define_symbol(tcc, "_GNU_SOURCE", "1");
#define xstr(s) str(s)
#define str(s) #s
  tcc_define_symbol(tcc, "PIXEL_TYPE", xstr(PIXEL_TYPE));
  tcc_define_symbol(tcc, "IPIXEL_TYPE", xstr(IPIXEL_TYPE));
#undef str
#undef xstr

  for (const struct symtab *sym = export_syms; sym->name; ++sym) {
    tcc_add_symbol(tcc, sym->name, sym->addr);
  }

  return tcc;
}

void Basic::init_tcc() {
  callvm = dcNewCallVM(4096);
  dcMode(callvm, DC_CALL_C_DEFAULT);

  // add empty module to make default symbols visible to BASIC
  modules.push_back(new_tcc());
}

static TCCState *current_tcc;

void Basic::itcc() {
  TCCState *tcc;
  if (current_tcc)
    tcc = current_tcc;
  else {
    tcc  = new_tcc();
    if (!tcc) {
      err = ERR_OOM;
      return;
    }
    current_tcc = tcc;
  }

  tcc_set_error_func(tcc, this, print_tcc_error);

  BString file = getParamFname();

  if (tcc_add_file(tcc, file.c_str()) < 0) {
    err = ERR_COMPILE;
    err_expected = "translation failed";
    tcc_delete(tcc);
    current_tcc = NULL;
    return;
  }

  modules.push_back(tcc);
}

void Basic::itcclink() {
  if (!current_tcc || tcc_relocate(current_tcc, TCC_RELOCATE_AUTO) < 0) {
    err = ERR_COMPILE;
    err_expected = "relocation failed";
    return;
  }

  modules.push_back(current_tcc);
  current_tcc = NULL;
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
