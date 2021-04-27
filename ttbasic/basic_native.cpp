// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include <vector>

#include "basic.h"
#include "basic_native.h"

#include "../libraries/tinycc/libtcc.h"
#include <dyncall.h>

struct module {
  TCCState *tcc;
  BString name;
};
std::vector<struct module> modules;

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
  struct module system = { new_tcc(), "system" };
  modules.push_back(system);
}

static TCCState *current_tcc;

void Basic::itcc() {
  TCCState *tcc;
  if (current_tcc)
    tcc = current_tcc;
  else {
    tcc = new_tcc();
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
  }
}

void Basic::itcclink() {
  if (!current_tcc) {
    err = ERR_COMPILE;
    err_expected = "nothing to link";
    return;
  }

  BString name;
  if (is_strexp())
    name = istrexp();
  else
    name = BString("mod") + BString(modules.size());

  if (tcc_get_symbol(current_tcc, "main") &&
      !tcc_get_symbol(current_tcc, name.c_str())) {
    // Add a wrapper function with the name of the module that takes a
    // variable number of string arguments, combines them into an argv array
    // and calls main().
    char *wrapper;
    asprintf(&wrapper,
             "#include <stdarg.h>"
             "extern int optind;"
             "int main(int argc, char **argv);"
             "int %s(const char *opt, ...) {"
             "        int argc = 2;"
             "        const char *args[16];"
             "        char *o;"
             "        args[0] = \"%s\";"
             "        args[1] = opt;"
             ""
             "        if (opt == 0) {"
             "                --argc;"
             "        } else {"
             "                va_list ap;"
             "                va_start(ap, opt);"
             "                while (argc < 16) {"
             "                        o = va_arg(ap, char *);"
             "                        if (o)"
             "                                args[argc++] = o;"
             "                        else"
             "                                break;"
             "                }"
             "        }"
             "        va_end(ap);"
             "        optind = 1;"
             "        return main(argc, args);"
             "}",
             name.c_str(), name.c_str());
    tcc_compile_string(current_tcc, wrapper);
  }

  if (tcc_relocate(current_tcc, TCC_RELOCATE_AUTO) < 0) {
    err = ERR_COMPILE;
    err_expected = "relocation failed";
    tcc_delete(current_tcc);
    current_tcc = NULL;
    return;
  }

  struct module new_mod = { current_tcc, name };
  modules.push_back(new_mod);

  current_tcc = NULL;
}

void *Basic::get_symbol(const char *sym_name) {
  for (auto mod : modules) {
    void *sym = tcc_get_symbol(mod.tcc, sym_name);
    if (sym)
      return sym;
  }
  return NULL;
}

const char *Basic::get_name(void *addr) {
  for (auto mod : modules) {
    const char *sym = tcc_get_name(mod.tcc, addr);
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

  dcArgPointer(callvm, NULL);

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
