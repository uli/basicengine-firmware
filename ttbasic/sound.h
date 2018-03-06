#include "SID.h"
#include "mml.h"

#define SOUND_CHANNELS	3

class BasicSound {
public:
    static void begin(void);
    static void playMml(int ch, const char *data);
    static void stopMml(int ch);
    static void pumpEvents();

    static SID sid;

    static inline bool isPlaying(int ch) {
        return m_next_event[ch] != 0;
    }
    static inline bool isFinished(int ch) {
        return m_finished[ch];
    }

private:
    static void defaults(int ch);
    static void ICACHE_RAM_ATTR mmlCallback(MML_INFO *p, void *extobj);

    static MML m_mml[SOUND_CHANNELS];
    static MML_OPTION m_mml_opt[SOUND_CHANNELS];
    static uint32_t m_next_event[SOUND_CHANNELS];

    static uint32_t m_sid_off[SOUND_CHANNELS];
    static bool m_finished[SOUND_CHANNELS];
};

extern BasicSound sound;
