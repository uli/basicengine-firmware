struct module {
  void *dl_handle;
  BString name;
};
extern std::vector<struct module> modules;

#include <list>
int exec_list(std::list<BString> &args);
