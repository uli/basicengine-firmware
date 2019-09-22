#ifndef __BASIC_H
#define __BASIC_H

#include "ttconfig.h"

#include <stdint.h>
#include <string.h>
#include <sdfiles.h>
#include "BString.h"

#include "colorspace.h"
#include "video.h"
#include "variable.h"
#include "proc.h"
#include "sound.h"

#define SIZE_LINE 256    // コマンドライン入力バッファサイズ + NULL
#define SIZE_IBUF 256    // 中間コード変換バッファサイズ

extern char lbuf[SIZE_LINE];
extern char tbuf[SIZE_LINE];
extern int32_t tbuf_pos;
extern unsigned char ibuf[SIZE_IBUF];

extern uint8_t err; // Error message index


// メモリ書き込みポインタのクリア
static inline void cleartbuf() {
  tbuf_pos=0;
  memset(tbuf,0,SIZE_LINE);
}


void c_putch(uint8_t c, uint8_t devno = 0);
void c_puts(const char *s, uint8_t devno=0);
void c_puts_P(const char *s, uint8_t devno=0);
void screen_putch(uint8_t c, bool lazy = false);
extern bool screen_putch_disable_escape_codes;

#define PRINT_P(msg) c_puts_P(PSTR((msg)))

#define dbg_printf(x, y...) printf_P(PSTR(x), y)

void putnum(num_t value, int8_t d, uint8_t devno=0);
void putint(int value, int8_t d, uint8_t devno=0);
void putHexnum(uint32_t value, uint8_t d, uint8_t devno=0);
uint16_t BASIC_INT hex2value(char c);

void newline(uint8_t devno=0);

#define NUM_FONTS 4

BString getParamFname();
num_t getparam();

void get_input(bool numeric = false, uint8_t eoi = '\r');

uint32_t getBottomLineNum();
uint32_t getTopLineNum();

BString getstr(uint8_t eoi = '\r');

num_t& BASIC_FP get_lvar(uint8_t arg);
BString& get_lsvar(uint8_t arg);

void BASIC_FP ivar();
void BASIC_FP ilvar();

#define MAX_VAR_NAME 32  // maximum length of variable names
#define SIZE_GSTK 10     // GOSUB stack size
#define SIZE_LSTK 10     // FOR stack size
#define SIZE_ASTK 16	// argument stack

#define MAX_RETVALS 4

#define basic_bool(x) ((x) ? -1 : 0)

#define NEW_ALL		0
#define NEW_PROG	1
#define NEW_VAR		2

extern void E_SYNTAX(unsigned char token);
#define SYNTAX_T(exp) do { static const char __msg[] PROGMEM = exp; \
                           err = ERR_SYNTAX; err_expected = __msg; \
                      } while(0)
extern void E_VALUE(int32_t from, int32_t to);

#define E_ERR(code, exp) do { \
  static const char __msg[] PROGMEM = exp; \
  err = ERR_ ## code; err_expected = __msg; \
  } while(0)

class Basic {
public:
  void basic();

  void draw_profile(void);
  void event_handle_sprite();
  void event_handle_pad();
  void event_handle_play(int ch);
  char* getLineStr(uint32_t lineno, uint8_t devno = 3);
  uint32_t getPrevLineNo(uint32_t lineno);
  uint32_t getNextLineNo(uint32_t lineno);

private:
  int list_free();
  unsigned char* getlp(uint32_t lineno);
  uint32_t getlineIndex(uint32_t lineno);
  uint8_t* getELSEptr(uint8_t* p, bool endif_only = false, int adjust = 0);
  uint8_t* getWENDptr(uint8_t* p);
  uint32_t countLines(uint32_t st = 0, uint32_t ed = UINT32_MAX);

  void inslist();
  void recalc_indent();
  uint8_t toktoi(bool find_prg_text = true);
  void irenum();

  int SMALL putlist(unsigned char* ip, uint8_t devno = 0);

  int get_array_dims(int *idxs);
  num_t getparam();

  void initialize_proc_pointers(void);
  void initialize_label_pointers(void);

  bool find_next_data();
  void data_push();
  void data_pop();

  void iprint(uint8_t devno = 0,uint8_t nonewln = 0);

  void do_trace();
  uint8_t ilrun();
  void clear_execution_state(bool clear);
  void irun(uint8_t* start_clp = NULL, bool cont = false, bool clear = true);

  void inew(uint8_t mode = NEW_ALL);

  bool get_range(uint32_t &start, uint32_t &end);

  void SMALL ilist(uint8_t devno=0, BString *search = NULL);

  void iloadbg();
  void isavebg();
  void isavepcx();
  void SMALL ildbmp();

  void imovebg();
  void imovesprite();

  num_t nvreg();
  int32_t ncharfun();

  void config_color();

  num_t nplay();

  uint8_t SMALL loadPrgText(char* fname, uint8_t newmode = NEW_ALL);
  BString sinput();

  void init_stack_frame();
  void push_num_arg(num_t n);
  void do_call(uint8_t proc_idx);
  void do_goto(uint32_t line);
  void do_gosub_p(unsigned char *lp, unsigned char *ip);
  void do_gosub(uint32_t lineno);
  void on_go(bool is_gosub, int cas);

  void SMALL error(uint8_t flgCmd = false);
  BString serror();
  void esyntax();
  void eunimp();

  BString ilrstr(bool right);

  typedef BString (Basic::*strfun_t)();
  static const strfun_t strfuntbl[];

  typedef void (Basic::*cmd_t)();
  static const Basic::cmd_t funtbl[];
  static const Basic::cmd_t funtbl_ext[];

  typedef num_t (Basic::*numfun_t)();
  static const Basic::numfun_t numfuntbl[];

#define DECL_FUNCS
#include "numfuntbl.h"
#include "strfuntbl.h"
#include "funtbl.h"
#undef DECL_FUNCS
#define esyntax_workaround esyntax

  bool is_strexp();
  BString istrvalue();
  BString istrexp();
  num_t irel_string();

  num_t nsvar_a(BString &);
  int get_num_local_offset(uint8_t arg, bool &is_local);
  num_t& get_lvar(uint8_t arg);
  int get_str_local_offset(uint8_t arg, bool &is_local);
  BString& get_lsvar(uint8_t arg);
  void set_svar(bool is_lsvar);

  num_t ivalue();
  num_t iexp();
  num_t imul();
  num_t iplus();
  num_t irel();
  num_t iand();

  num_t nsys();
  int32_t ipeek(int type);
  void do_poke(int type);

  void isetDate();
  void igetDate();
  void igetTime();

  void iformat();
  void iflash();

  int get_filenum_param();

  void isetap();
  void inetopen();
  void inetclose();
  void inetget();
  BString snetinput();
  BString snetget();

  unsigned char* iexe(int stk = -1);
  uint8_t SMALL icom();

  // '('チェック関数
  inline uint8_t checkOpen() {
    if (*cip != I_OPEN) err = ERR_PAREN;
    else cip++;
    return err;
  }

  // ')'チェック関数
  inline uint8_t checkClose() {
    if (*cip != I_CLOSE) err = ERR_PAREN;
    else cip++;
    return err;
  }

#ifdef FLOAT_NUMS
  uint8_t BASIC_FP getParam(int32_t& prm, token_t next_token);
  uint8_t BASIC_FP getParam(int32_t& prm, int32_t v_min,  int32_t v_max, token_t next_token);
#endif

  // コマンド引数取得(int32_t,引数チェックあり)
  uint8_t BASIC_FP getParam(num_t& prm, num_t v_min,  num_t v_max, token_t next_token);
  uint32_t BASIC_FP getParam(uint32_t& prm, uint32_t v_min, uint32_t v_max, token_t next_token);
  uint8_t BASIC_FP getParam(uint32_t& prm, token_t next_token);
  uint8_t BASIC_FP getParam(num_t& prm, token_t next_token);

  BString getParamFname();

  inline bool end_of_statement()
  {
    return *cip == I_EOL || *cip == I_COLON || *cip == I_ELSE || *cip == I_IMPLICITENDIF || *cip == I_SQUOT;
  }

  unsigned char ibuf[SIZE_IBUF];    // i-code conversion buffer

  int size_list;

  NumVariables nvar;
  VarNames nvar_names;
  StringVariables svar;
  VarNames svar_names;

  #define MAX_ARRAY_DIMS 4
  NumArrayVariables<num_t> num_arr;
  VarNames num_arr_names;
  StringArrayVariables<BString> str_arr;
  VarNames str_arr_names;
  BasicListVariables<BString> str_lst;
  VarNames str_lst_names;
  BasicListVariables<num_t> num_lst;
  VarNames num_lst_names;

  VarNames proc_names;
  Procedures procs;
  VarNames label_names;
  Labels labels;

  unsigned char *listbuf; // Pointer to program list area

  unsigned char* clp;               // Pointer current line
  unsigned char* cip;               // Pointer current Intermediate code
  struct {
    uint8_t *lp;
    uint8_t *ip;
    uint8_t num_args;
    uint8_t str_args;
    uint8_t proc_idx;
  } gstk[SIZE_GSTK];   // GOSUB stack
  unsigned char gstki;              // GOSUB stack index

  // Arguments/locals stack
  num_t astk_num[SIZE_ASTK];
  unsigned char astk_num_i;
  BString astk_str[SIZE_ASTK];
  unsigned char astk_str_i;

  struct {
    uint8_t *lp;
    uint8_t *ip;
    num_t vto;
    num_t vstep;
    int16_t index;
    bool local;
  } lstk[SIZE_LSTK];   // loop stack
  unsigned char lstki;              // loop stack index

  uint8_t *cont_clp = NULL;
  uint8_t *cont_cip = NULL;

  num_t retval[MAX_RETVALS];        // multi-value returns (numeric)
  BString retstr[MAX_RETVALS];	    // multi-value returns (string)

  bool event_error_enabled;
  unsigned char *event_error_lp;
  unsigned char *event_error_ip;
  unsigned char *event_error_resume_lp;
  unsigned char *event_error_resume_ip;

  bool math_exceptions_disabled;

  unsigned char *data_lp;
  unsigned char *data_ip;
  bool in_data;
};

extern Basic *bc;

int token_size(uint8_t *code);

// キーワードテーブル
#include "kwtbl.h"

// Keyword count
#define SIZE_KWTBL (sizeof(kwtbl) / sizeof(const char*))
#define SIZE_KWTBL_EXT (sizeof(kwtbl_ext) / sizeof(const char *))

// i-code(Intermediate code) assignment
#include "kwenum.h"

extern uint8_t err;
extern const char *err_expected;

void* BASIC_INT sanitize_addr(uint32_t vadr, int type);

#ifdef ESP8266
extern "C" size_t umm_free_heap_size( void );
#else
extern int try_malloc();
#endif

#define MAX_USER_FILES 16
struct FILEDIR {
  FILE *f;
  DIR *d;
  BString dir_name;
};
extern FILEDIR user_files[MAX_USER_FILES];

extern sdfiles bfs;

#include <tscreenBase.h>
#include <tTVscreen.h>

extern tTVscreen sc0;

extern int redirect_output_file;
extern int redirect_input_file;

static inline uint16_t c_getch() {
  if (redirect_input_file >= 0)
    return getc(user_files[redirect_input_file].f);
  else
    return sc0.get_ch();
}

#define c_kbhit( ) sc0.isKeyIn()

#include <TKeyboard.h>
extern TKeyboard kb;
uint8_t BASIC_FP process_hotkeys(uint16_t c, bool dont_dump = false);

num_t BASIC_FP ivalue();

void iloadbg();

int32_t ncharfun();
num_t nvreg();
num_t nplay();

#define COL_BG		0
#define COL_FG		1
#define COL_KEYWORD	2
#define COL_LINENUM	3
#define COL_NUM		4
#define COL_VAR		5
#define COL_LVAR	6
#define COL_OP		7
#define COL_STR		8
#define COL_PROC	9
#define COL_COMMENT	10
#define COL_BORDER	11

#define CONFIG_COLS	12

typedef struct {
  int8_t NTSC;        // NTSC設定 (0,1,2,3)
  int8_t line_adjust;	// number of lines to add to default
  int16_t KEYBOARD;    // キーボード設定 (0:JP, 1:US)
  uint8_t color_scheme[CONFIG_COLS][3];
  bool interlace;
  bool lowpass;
  uint8_t mode;
  uint8_t font;
  pixel_t cursor_color;
  uint8_t beep_volume;
} SystemConfig;
extern SystemConfig CONFIG;

void loadConfig();
void iloadconfig();

#define COL(n)	(csp.colorFromRgb(CONFIG.color_scheme[COL_ ## n]))

extern bool event_play_enabled;
extern uint8_t event_play_proc_idx[SOUND_CHANNELS];

#define MAX_PADS 3
extern bool event_pad_enabled;
extern uint8_t event_pad_proc_idx[MAX_PADS];
extern int event_pad_last[MAX_PADS];

extern uint8_t event_sprite_proc_idx;

#ifdef USE_BG_ENGINE
extern bool restore_text_window;
extern bool restore_bgs;
#endif

void SMALL resize_windows();
void SMALL restore_windows();

void BASIC_FP init_stack_frame();
void BASIC_FP push_num_arg(num_t n);

extern const uint8_t *fonts[NUM_FONTS];

void BASIC_FP process_events(void);

void basic_init_io();
void basic_init_input();
void basic_init_file_early();
void basic_init_file_late();

#endif
