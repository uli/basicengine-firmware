#include "mml.h"

#include <sdfiles.h>

#define TSF_NO_STDIO
#include "tsf.h"
#include "nosdki2s.h"

#define SOUND_CHANNELS	3

class BasicSound {
public:
    static void begin(void);
    static void playMml(int ch, const char *data);
    static void stopMml(int ch);
    static void pumpEvents();
    static void render();

    static inline bool isPlaying(int ch) {
        return m_next_event[ch] != 0;
    }
    static inline bool isFinished(int ch) {
        return m_finished[ch];
    }

    static inline bool needSamples() {
        return nosdk_i2s_curr_buf_pos == 0;
    }
    
    static inline int instCount() {
        if (m_tsf)
          return tsf_get_presetcount(m_tsf);
        else
          return 0;
    }
    static BString instName(int inst);
    static void noteOn(int ch, int inst, int note, float vel, int ticks);
    
private:
    static tsf *m_tsf;
    static struct tsf_stream m_sf2;
    static Unifile m_sf2_file;
    static int tsfile_read(void *data, void *ptr, unsigned int size);
    static int tsfile_tell(void *data);
    static int tsfile_skip(void *data, unsigned int count);
    static int tsfile_seek(void *data, unsigned int pos);
    static int tsfile_close(void *data);
    static int tsfile_size(void *data);

    static void defaults(int ch);
    static void ICACHE_RAM_ATTR mmlCallback(MML_INFO *p, void *extobj);

    static MML m_mml[SOUND_CHANNELS];
    static MML_OPTION m_mml_opt[SOUND_CHANNELS];
    static uint32_t m_next_event[SOUND_CHANNELS];

    static uint32_t m_off_time[SOUND_CHANNELS];
    static uint8_t m_off_key[SOUND_CHANNELS];
    static uint8_t m_off_inst[SOUND_CHANNELS];
    static uint8_t m_ch_inst[SOUND_CHANNELS];
    static bool m_finished[SOUND_CHANNELS];
};

extern BasicSound sound;
