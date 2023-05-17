// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include <vector>

#include "basic.h"
#include "basic_native.h"
#include "eb_native.h"

#include <dyncall.h>

std::vector<struct module> modules;

void print_tcc_error(void *b, const char *msg) {
  c_puts(msg); newline();
}

int output_type = TCC_OUTPUT_MEMORY;

static TCCState *new_tcc(bool init_syms = true) {
  TCCState *tcc = eb_tcc_new(output_type);
  if (!tcc)
    return NULL;

  if (init_syms)
    eb_tcc_initialize_symbols(tcc);

  return tcc;
}

void Basic::init_tcc() {
  callvm = dcNewCallVM(4096);
  dcMode(callvm, DC_CALL_C_DEFAULT);

  // add empty module to make default symbols visible to BASIC
  if (modules.size() == 0) {
    struct module system = { new_tcc(), "system", NULL, 0 };
    modules.push_back(system);
  }
}

static TCCState *current_tcc;

void Basic::itcc() {
  TCCState *tcc;
  if (current_tcc)
    tcc = current_tcc;
  else {
    tcc = eb_tcc_new(output_type);
    if (!tcc) {
      err = ERR_OOM;
      return;
    }

    if (output_type == TCC_OUTPUT_MEMORY)
      eb_tcc_initialize_symbols(tcc);

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

  if (eb_tcc_link(current_tcc, name.c_str(), output_type) < 0) {
    tcc_delete(current_tcc);
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
  for(int i = modules.size(); i-- > 0; ) {
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

std::vector<void (*)(void)> atexit_funcs;

extern "C" void be__exit(int ret) {
  atexit_funcs.clear();
  // XXX: How do we know what our BASIC context is?
  bc->exit(ret);
}

extern "C" void be_exit(int ret) {
  for (auto a : atexit_funcs)
    a();

  be__exit(ret);
}

extern "C" void be_atexit(void (*function)(void)) {
  atexit_funcs.push_back(function);
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

int exec_list(std::list<BString> &args) {
#ifdef __unix__
  std::vector<const char *> argsp;

  for (auto &s : args)
    argsp.push_back(s.c_str());

  argsp.push_back(NULL);

  return execvp(argsp[0], (char *const *)argsp.data());
#else
  return -1;
#endif
}
