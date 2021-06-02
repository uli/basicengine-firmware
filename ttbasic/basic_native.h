#include "../libraries/tinycc/libtcc.h"

struct symtab {
    const char *name;
    const void *addr;
};

extern const struct symtab export_syms[];

#ifdef __cplusplus
struct module {
  TCCState *tcc;
  BString name;
  void *init_data;
  int init_len;
};
extern std::vector<struct module> modules;
#endif
