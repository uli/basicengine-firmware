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
  void *init_data;
  int init_len;
};
std::vector<struct module> modules;

extern "C" void print_tcc_error(void *b, const char *msg) {
  c_puts(msg); newline();
}

int output_type = TCC_OUTPUT_MEMORY;

static TCCState *new_tcc(bool init_syms = true) {
  TCCState *tcc = tcc_new();
  if (!tcc)
    return NULL;

  tcc_set_output_type(tcc, output_type);
  tcc_set_options(tcc, "-nostdlib");

  BString default_include_path =
#ifdef SDL
    BString(getenv("ENGINEBASIC_ROOT")) +
#endif
    BString("/include");
  tcc_add_include_path(tcc, default_include_path.c_str());

  tcc_define_symbol(tcc, "ENGINEBASIC", "1");
  tcc_define_symbol(tcc, "_GNU_SOURCE", "1");
  tcc_define_symbol(tcc, "__DYNAMIC_REENT__", "1");
#define xstr(s) str(s)
#define str(s) #s
  tcc_define_symbol(tcc, "PIXEL_TYPE", xstr(PIXEL_TYPE));
  tcc_define_symbol(tcc, "IPIXEL_TYPE", xstr(IPIXEL_TYPE));
#undef str
#undef xstr

  if (init_syms) {
    for (const struct symtab *sym = export_syms; sym->name; ++sym) {
      tcc_add_symbol(tcc, sym->name, sym->addr);
    }
  }

  return tcc;
}

void Basic::init_tcc() {
  callvm = dcNewCallVM(4096);
  dcMode(callvm, DC_CALL_C_DEFAULT);

  // add empty module to make default symbols visible to BASIC
  struct module system = { new_tcc(), "system", NULL, 0 };
  modules.push_back(system);

#ifdef SDL
  setenv("HOME", getenv("ENGINEBASIC_ROOT"), 1);
#else
  setenv("HOME", "/", 1);
#endif
  setenv("TERM", "ansi", 1);
}

static TCCState *current_tcc;

void Basic::itcc() {
  TCCState *tcc;
  if (current_tcc)
    tcc = current_tcc;
  else {
    tcc = new_tcc(output_type == TCC_OUTPUT_MEMORY);
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
             "#include <string.h>"
             "#include <unistd.h>"
             "extern int optind;"
             "extern char _etext, _end;"
             "volatile void *_initial_data;"
             "int main(int argc, char **argv, char **environ);"
             "int %s(const char *opt, ...) {"
             "        int argc = 2;"
             "        const char *args[16];"
             "        char *o;"
             "        volatile void *tmp;"
             "        args[0] = \"%s\";"
             "        args[1] = opt;"
             ""
             "        if (opt == 0) {"
             "                --argc;"
             "        } else {"
             "                va_list ap;"
             "                va_start(ap, opt);"
             "                while (argc < 15) {"
             "                        o = va_arg(ap, char *);"
             "                        if (o)"
             "                                args[argc++] = o;"
             "                        else"
             "                                break;"
             "                }"
             "        }"
             "        va_end(ap);"
             "        args[argc] = NULL;"
             "        optind = 1;"
             "        tmp = _initial_data;"
             "        memcpy(&_etext, (void *)_initial_data, &_end - &_etext);"
             "        _initial_data = tmp;"
             "        return main(argc, args, environ);"
             "}",
             name.c_str(), name.c_str());
    tcc_compile_string(current_tcc, wrapper);
  }

  if (output_type == TCC_OUTPUT_MEMORY) {
    if (tcc_relocate(current_tcc, TCC_RELOCATE_AUTO) < 0) {
      err = ERR_COMPILE;
      err_expected = "relocation failed";
      tcc_delete(current_tcc);
      current_tcc = NULL;
      return;
    }

    char *etext = (char *)tcc_get_symbol(current_tcc, "_etext");
    char *end   = (char *)tcc_get_symbol(current_tcc, "_end");

    char *initial_data = (char *)malloc(end - etext);
    memcpy(initial_data, etext, end - etext);

    void **init_ptr = (void **)tcc_get_symbol(current_tcc, "_initial_data");
    if (init_ptr)
      *init_ptr = initial_data;

    struct module new_mod = { current_tcc, name, initial_data, (int)(end - etext) };
    modules.push_back(new_mod);
  } else {
    if (tcc_output_file(current_tcc, name.c_str()) < 0) {
      err = ERR_COMPILE;
      err_expected = "failed to output file";
      tcc_delete(current_tcc);
    }
    output_type = TCC_OUTPUT_MEMORY;
  }

  current_tcc = NULL;
}

void Basic::itccmode() {
  int32_t mode;
  if (getParam(mode, 1, 4, I_NONE))
   return;
  output_type = mode;
}

void *Basic::get_symbol(const char *sym_name) {
  for(int i = modules.size(); i-- > 0; --i) {
    void *sym = tcc_get_symbol(modules[i].tcc, sym_name);
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

Basic::nfc_result Basic::do_nfc(void *sym, enum return_type rtype) {
  static float empty[16] = {};
  nfc_result ret;

  dcReset(callvm);

  std::vector<char *> string_list;

  if (!end_of_statement()) {
    do {
      if (*cip == I_SHARP) {
        ++cip;
        double arg = iexp();
        if (err)
          goto out;
        dcArgDouble(callvm, arg);
      } else if (*cip == I_BANG) {
        ++cip;
        float arg = iexp();
        if (err)
          goto out;
        dcArgFloat(callvm, arg);
      } else if (*cip == I_MUL) {
        ++cip;
        void *arg = (void *)(uintptr_t)iexp();
        if (err)
          goto out;
        dcArgPointer(callvm, arg);
      } else if (is_strexp()) {
        BString arg_str = istrexp();
        char *arg = strdup(arg_str.c_str());
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

  switch (rtype) {
    case RET_INT: ret.rint = dcCallInt(callvm, sym); break;
    case RET_UINT: ret.ruint = dcCallInt(callvm, sym); break;
    case RET_POINTER: ret.rptr = dcCallPointer(callvm, sym); break;
    case RET_FLOAT: ret.rflt = dcCallFloat(callvm, sym); break;
    case RET_DOUBLE: ret.rdbl = dcCallDouble(callvm, sym); break;
  }

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

  return_type rtype = RET_INT;

  if (*cip == I_SHARP) {
    ++cip;
    rtype = RET_DOUBLE;
  } else if (*cip == I_BANG) {
    ++cip;
    rtype = RET_FLOAT;
  } else if (*cip == I_MUL) {
    ++cip;
    rtype = RET_POINTER;
  };

  if (checkOpen())
    return 0;

  nfc_result ret = do_nfc(sym, rtype);

  if (checkClose())
    return 0;

  switch (rtype) {
    case RET_INT: return ret.rint;
    case RET_UINT: return ret.ruint;
    case RET_POINTER: return (uintptr_t)ret.rptr;
    case RET_FLOAT: return ret.rflt;
    case RET_DOUBLE: return ret.rdbl;
    default: err = ERR_SYS; return 0;
  }
}

BString Basic::snfc() {
  BString out;

  void *sym = *((void **)cip);
  cip += icodes_per_ptr();

  return_type rtype = RET_POINTER;
  if (*cip == I_MUL)
    ++cip;

  if (checkOpen())
    return out;

  nfc_result ret = do_nfc(sym, rtype);

  if (checkClose())
    return out;

  out = (char *)ret.rptr;
  return out;
}

extern "C" void be_exit(int ret) {
  // XXX: How do we know what our BASIC context is?
  bc->exit(ret);
}

/***bf sys GETSYM
Returns a pointer to a native symbol.
\usage ptr = GETSYM(name$)
\args
@name$	name of a native symbol
\ret Pointer to requested symbol, or 0 if not found.
***/
num_t Basic::ngetsym() {
  if (checkOpen())
    return 0;
  BString sym = istrexp();
  if (checkClose())
    return 0;

  return (uintptr_t)get_symbol(sym.c_str());
}
