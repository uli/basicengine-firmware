#include "../libraries/tinycc/libtcc.h"

struct symtab {
    const char *name;
    const void *addr;
};

extern const struct symtab export_syms[];

#ifdef __cplusplus
extern "C"
#endif
void print_tcc_error(void *b, const char *msg);

#ifdef __cplusplus
struct module {
  void *dl_handle;
  BString name;
};
extern std::vector<struct module> modules;

#include <list>
int exec_list(std::list<BString> &args);
#endif
