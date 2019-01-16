#define PROGMEM

#define PSTR(x) (x)

#define strncpy_P strncpy
#define memcpy_P memcpy
#define memmem_P memmem
#define sprintf_P sprintf
#define strlen_P strlen
#define strcpy_P strcpy
#define strcat_P strcat
#define strncasecmp_P strncasecmp
#define strcmp_P strcmp

#define pgm_read_byte(p) (*((const uint8_t *)(p)))
#define pgm_read_word(p) (*((const uint16_t *)(p)))
#define pgm_read_dword(p) (*((const uint32_t *)(p)))

typedef char prog_char;

#define PGM_P char*
