// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include <vector>

#include "basic.h"
#include "basic_native.h"

#include <dyncall.h>
#include <dlfcn.h>

std::vector<struct module> modules;

void Basic::init_tcc() {
  callvm = dcNewCallVM(4096);
  dcMode(callvm, DC_CALL_C_DEFAULT);

  // add empty module to make default symbols visible to BASIC
  if (modules.size() == 0) {
    struct module system = { NULL, "system" };
    modules.push_back(system);
  }
}

void Basic::itcc() {
  std::list<BString> args;
  args.push_back("tcc");
  args.push_back("-I");
  args.push_back(BString(getenv("ENGINEBASIC_ROOT")) + BString("/sys/include"));

  for (;;) {
    BString file = getParamFname();
    if (err)
      return;
    args.push_back(file);
    if (*cip == I_COMMA)
      ++cip;
    else
      break;
  }

  if (*cip == I_TO) {
    ++cip;
    BString outfile = getParamFname();
    args.push_back("-o");
    args.push_back(outfile);
  }

  if (*cip == I_MOD) {
    ++cip;
    args.push_back("-shared");
  }

  run_list(args);
}

void *Basic::get_symbol(const char *sym_name) {
  dlerror();

  void *sym = dlsym(RTLD_DEFAULT, sym_name);

  if (!dlerror())
    return sym;
  else
    return NULL;
}

const char *Basic::get_name(void *addr) {
  static Dl_info info;
  if (!dladdr(addr, &info))
    return NULL;

  // XXX: returns symbol even if addr is not the start address; do we want that?
  return info.dli_sname;
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

#include <sys/wait.h>

int exec_list(std::list<BString> &args) {
  std::vector<const char *> argsp;

  for (auto &s : args)
    argsp.push_back(s.c_str());

  argsp.push_back(NULL);

  return execvp(argsp[0], (char *const *)argsp.data());
}

int run_list(std::list<BString> &args) {
  pid_t pid = fork();
  if (pid < 0) {
    return -1;
  } else if (pid > 0) {
    int wstatus;
    waitpid(pid, &wstatus, 0);
    return WEXITSTATUS(wstatus);
  } else {
    exec_list(args);
    return -1;
  }
}
