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
  TCCState *tcc;
  BString name;
  void *init_data;
  int init_len;
};
extern std::vector<struct module> modules;

#include <list>
int exec_list(std::list<BString> &args);
int run_list(std::list<BString> &args);
void shell_list(std::list<BString> &args);
#endif
