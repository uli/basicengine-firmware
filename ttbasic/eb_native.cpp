// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"
#include "basic_native.h"
#include "eb_native.h"

TCCState *eb_tcc_new(int output_type) {
  TCCState *tcc = tcc_new();
  if (!tcc)
    return NULL;

  tcc_set_output_type(tcc, output_type);
  tcc_set_options(tcc, "-nostdlib");

  BString default_include_path =
#ifdef SDL
          BString(getenv("ENGINEBASIC_ROOT")) +
#endif
          BString("/sys/include");
  tcc_add_include_path(tcc, default_include_path.c_str());
  tcc_add_include_path(tcc, (default_include_path + "/tcc").c_str());

  tcc_define_symbol(tcc, "ENGINEBASIC", "1");
  tcc_define_symbol(tcc, "_GNU_SOURCE", "1");
  tcc_define_symbol(tcc, "__DYNAMIC_REENT__", "1");
#define xstr(s) str(s)
#define str(s)  #s
  tcc_define_symbol(tcc, "PIXEL_TYPE", xstr(PIXEL_TYPE));
  tcc_define_symbol(tcc, "IPIXEL_TYPE", xstr(IPIXEL_TYPE));
#undef str
#undef xstr

  return tcc;
}

void eb_tcc_initialize_symbols(TCCState *tcc) {
  for (const struct symtab *sym = export_syms; sym->name; ++sym) {
    tcc_add_symbol(tcc, sym->name, sym->addr);
  }
}

struct add_exports_cb_context {
  TCCState *_new;
  TCCState *module;
};

// Adds exports from other modules to the current context.
void add_exports_cb(const char *name, void *addr, void *userdata) {
  auto tcc = (struct add_exports_cb_context *)userdata;

  if (!strncmp(name, "__export_", 9) && !tcc_have_symbol(tcc->_new, name + 9)) {
    name += 9;
    void *module_addr = tcc_get_symbol(tcc->module, name);
    tcc_add_symbol(tcc->_new, name, module_addr);
  }
};

int eb_tcc_link(TCCState *tcc, const char *name, int output_type) {
  if (tcc_have_symbol(tcc, "main") && !tcc_have_symbol(tcc, name)) {
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
             name, name);
    tcc_compile_string(tcc, wrapper);
  }

  if (output_type == TCC_OUTPUT_MEMORY) {
    // Add symbols exported from other modules before linking.
    for(int i = modules.size(); i-- > 1; ) {
      struct add_exports_cb_context ctx = { tcc, modules[i].tcc };
      tcc_enumerate_symbols(modules[i].tcc, add_exports_cb, &ctx);
    }

    if (tcc_relocate(tcc, TCC_RELOCATE_AUTO) < 0) {
      err = ERR_COMPILE;
      err_expected = "relocation failed";
      return -1;
    }

    auto initcall = (void (*)(void))tcc_get_symbol(tcc, "__initcall");
    if (initcall)
      initcall();

    char *etext = (char *)tcc_get_symbol(tcc, "_etext");
    char *end = (char *)tcc_get_symbol(tcc, "_end");

    char *initial_data = (char *)malloc(end - etext);
    memcpy(initial_data, etext, end - etext);

    void **init_ptr = (void **)tcc_get_symbol(tcc, "_initial_data");
    if (init_ptr)
      *init_ptr = initial_data;

    struct module new_mod = { tcc, name, initial_data, (int)(end - etext) };
    modules.push_back(new_mod);
  } else {
    if (tcc_output_file(tcc, name) < 0) {
      err = ERR_COMPILE;
      err_expected = "failed to output file";
      return -1;
    }
  }
  return 0;
}
