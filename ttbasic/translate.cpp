#include "basic.h"

struct msg {
    const char *msg;
    const char *trans;
};

const struct msg *translations[] = {
};

const char *gettext(const char *msg) {
    return msg;
}
