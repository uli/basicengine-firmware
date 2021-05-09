#include "basic.h"

struct msg {
    const char *msg;
    const char *trans;
};

#include "msgs_de.h"
#include "msgs_fr.h"
#include "msgs_es.h"

const struct msg *translations[] = {
    msgs_de,
    msgs_fr,
    msgs_es,
};

const char *gettext(const char *msg) {
    if (CONFIG.language == 0)
        return msg;

    const struct msg *msgs = translations[CONFIG.language - 1];

    for (const struct msg *trans_msg = msgs; trans_msg->msg; ++trans_msg) {
        if (!strcmp(msg, trans_msg->msg))
            return trans_msg->trans;
    }
    return msg;
}
