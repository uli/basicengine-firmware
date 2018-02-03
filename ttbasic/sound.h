#include "SID.h"
#include "mml.h"

class BasicSound {
public:
    static void begin(void);
    static void playMml(const char *data);
    static void stopMml();
    static void pumpEvents();

    static SID sid;

private:
    static void defaults();
    static void ICACHE_RAM_ATTR mmlCallback(MML_INFO *p, void *extobj);

    static MML m_mml;
    static MML_OPTION m_mml_opt;
    static uint32_t m_next_event;

    static uint32_t m_sid_off[3];
};

extern BasicSound sound;
