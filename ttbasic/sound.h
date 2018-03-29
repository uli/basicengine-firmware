#include "mml.h"

#include "basic.h"

//#define DEBUG_SOUND

#ifdef DEBUG_SOUND
//#define DEBUG_TSF_MEMORY
#define dbg_snd(x...) dbg_printf("snd: " x)
#else
#define dbg_snd(x...) do {} while(0)
#endif

#ifdef DEBUG_TSF_MEMORY
static inline void *dbg_malloc(size_t s, int line) {
  void *x = malloc(s);
  printf("malloc %d@%d -> %p\r\n", s, line, x);
  return x;
}
#define TSF_MALLOC(a) dbg_malloc((a), __LINE__)
#define TSF_REALLOC(a, b) (printf("realloc %p -> %d@%d\r\n", (a), (b), __LINE__), realloc((a),(b)))
#define TSF_FREE(a) do { printf("free %p@%d\r\n", (a), __LINE__); free((a)); } while(0)
#endif

#define TSF_NO_STDIO
#include "tsf.h"
extern "C" {
#include "nosdki2s.h"
};

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
        if (!m_tsf)
          loadFont();
        if (m_tsf)
          return tsf_get_presetcount(m_tsf);
        else
          return 0;
    }
    static BString instName(int inst);
    static void noteOn(int ch, int inst, int note, float vel, int ticks);

    static void loadFont();
    static void unloadFont();
    static inline void setFontName(BString &n) {
      m_font_name = n;
    }

    static void beep(int period, int vol = 15, const uint8_t *env = NULL);
    static void noBeep();

private:
    static void setBeep(int period, int vol);

    static const uint8_t *m_beep_env;
    static uint16_t m_beep_period;
    static uint8_t m_beep_vol;

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
    static uint32_t mmlGetNoteLength(int ch, uint32_t note_ticks);

    static MML m_mml[SOUND_CHANNELS];
    static MML_OPTION m_mml_opt[SOUND_CHANNELS];
    static uint32_t m_next_event[SOUND_CHANNELS];

    static uint32_t m_off_time[SOUND_CHANNELS];
    static uint8_t m_off_key[SOUND_CHANNELS];
    static uint8_t m_off_inst[SOUND_CHANNELS];
    static uint8_t m_ch_inst[SOUND_CHANNELS];
    static bool m_finished[SOUND_CHANNELS];
    static uint16_t m_bpm[SOUND_CHANNELS];
    static uint32_t m_all_done_time;
    static BString m_font_name;
};

extern BasicSound sound;
