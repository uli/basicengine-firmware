
/* Jzip V2.1 Infocom/Inform Zcode Format Interpreter
 * --------------------------------------------------------------------
 * Copyright (c) 2000  John D. Holder.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *       
 * --------------------------------------------------------------------
 */
 
 /*
  * Modified by Louis Davis April, 2012
  * Arduino support.
  *
  */
  
 /*
  * ztypes.h
  *
  * Any global stuff required by the C modules.
  *
  */

#if !defined(__ZTYPES_INCLUDED)
#define __ZTYPES_INCLUDED

#include <assert.h>
#include <avr/pgmspace.h>
#include <Arduino.h>

/* Configuration options */

#define DEFAULT_ROWS  2         /* Default screen height */
#define DEFAULT_COLS 40         /* Deafult screen width */

#define RANDOM_FUNC  random
#define SRANDOM_FUNC srandom

/* Global defines */

#ifndef UNUSEDVAR
#define UNUSEDVAR(a)  a=a;
#endif

/* number of bits in a byte.  needed by AIX!!! ;^) */
#ifndef NBBY
#define NBBY 8
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

/* Z types */

typedef unsigned char zbyte_t;  /* unsigned 1 byte quantity */
typedef unsigned short zword_t; /* unsigned 2 byte quantity */
typedef short ZINT16;           /*   signed 2 byte quantity */

#define H_TYPE                 0
#define H_CONFIG               1

#define CONFIG_BYTE_SWAPPED 0x01 /* Game data is byte swapped          - V3   */
#define CONFIG_TIME         0x02 /* Status line displays time          - V3   */
#define CONFIG_MAX_DATA     0x04 /* Data area should 64K if possible   - V3   */
#define CONFIG_TANDY        0x08 /* Tandy licensed game                - V3   */
#define CONFIG_NOSTATUSLINE 0x10 /* Interp can't support a status line - V3   */
#define CONFIG_WINDOWS      0x20 /* Interpr supports split screen mode - V3   */

#define CONFIG_COLOUR       0x01 /* Game supports colour               - V5+  */
#define CONFIG_PICTURES     0x02 /* Picture displaying available?      - V4+  */
#define CONFIG_BOLDFACE     0x04 /* Interpr supports boldface style    - V4+  */
#define CONFIG_EMPHASIS     0x08 /* Interpreter supports text emphasis - V4+  */
#define CONFIG_FIXED        0x10 /* Interpr supports fixed width style - V4+  */
#define CONFIG_SFX          0x20 /* Interpr supports sound effects     - V4+  */
#define CONFIG_TIMEDINPUT   0x80 /* Interpr supports timed input       - V4+  */

#define CONFIG_PROPORTIONAL 0x40 /* Interpr uses proportional font     - V3+  */


#define H_VERSION              2
#define H_DATA_SIZE            4
#define H_START_PC             6
#define H_WORDS_OFFSET         8
#define H_OBJECTS_OFFSET      10
#define H_GLOBALS_OFFSET      12
#define H_RESTART_SIZE        14
#define H_FLAGS               16

#define SCRIPTING_FLAG      0x01
#define FIXED_FONT_FLAG     0x02
#define REFRESH_FLAG        0x04
#define GRAPHICS_FLAG       0x08
#define SOUND_FLAG          0x10 /* V4 */
#define UNDO_AVAILABLE_FLAG 0x10 /* V5 */
#define COLOUR_FLAG         0x40
#define NEW_SOUND_FLAG      0x80

#define H_RELEASE_DATE        18
#define H_SYNONYMS_OFFSET     24
#define H_FILE_SIZE           26
#define H_CHECKSUM            28
#define H_INTERPRETER         30
#define H_UNICODE_TABLE       34

#define INTERP_GENERIC 0
#define INTERP_DEC_20 1
#define INTERP_APPLE_IIE 2
#define INTERP_MACINTOSH 3
#define INTERP_AMIGA 4
#define INTERP_ATARI_ST 5
#define INTERP_MSDOS 6
#define INTERP_CBM_128 7
#define INTERP_CBM_64 8
#define INTERP_APPLE_IIC 9
#define INTERP_APPLE_IIGS 10
#define INTERP_TANDY 11
#define INTERP_UNIX 12
#define INTERP_VMS 13

#define H_INTERPRETER_VERSION 31
#define H_SCREEN_ROWS 32
#define H_SCREEN_COLUMNS 33
#define H_SCREEN_LEFT 34
#define H_SCREEN_RIGHT 35
#define H_SCREEN_TOP 36
#define H_SCREEN_BOTTOM 37
#define H_MAX_CHAR_WIDTH 38
#define H_MAX_CHAR_HEIGHT 39
#define H_FILLER1 40

#define H_FUNCTION_KEYS_OFFSET 46
#define H_FILLER2 48

#define H_STANDARD_HIGH 50
#define H_STANDARD_LOW 51

#define H_ALTERNATE_ALPHABET_OFFSET 52
#define H_MOUSE_POSITION_OFFSET 54
#define H_FILLER3 56

#define V1 1

#define V2 2

/* Version 3 object format */

#define V3 3

#define O3_ATTRIBUTES 0
#define O3_PARENT 4
#define O3_NEXT 5
#define O3_CHILD 6
#define O3_PROPERTY_OFFSET 7

#define O3_SIZE 9

#define PARENT3(offset) (offset + O3_PARENT)
#define NEXT3(offset) (offset + O3_NEXT)
#define CHILD3(offset) (offset + O3_CHILD)

#define P3_MAX_PROPERTIES 0x20

/* Version 4 object format */

#define V4 4

#define O4_ATTRIBUTES 0
#define O4_PARENT 6
#define O4_NEXT 8
#define O4_CHILD 10
#define O4_PROPERTY_OFFSET 12

#define O4_SIZE 14

#define PARENT4(offset) (offset + O4_PARENT)
#define NEXT4(offset) (offset + O4_NEXT)
#define CHILD4(offset) (offset + O4_CHILD)

#define P4_MAX_PROPERTIES 0x40

#define V5 5
#define V6 6
#define V7 7
#define V8 8

/* Interpreter states */

#define STOP 0
#define RUN 1

/* Call types */

#define FUNCTION 0x0000

#define PROCEDURE 0x0100
#define ASYNC 0x0200

#define ARGS_MASK 0x00ff
#define TYPE_MASK 0xff00


/* Local defines */

#define STACK_SIZE 256

#define ON     1
#define OFF    0
#define RESET -1

#define Z_SCREEN      255
#define TEXT_WINDOW   0
#define STATUS_WINDOW 1

#define MIN_ATTRIBUTE 0
#define NORMAL        0
#define REVERSE       1
#define BOLD          2
#define EMPHASIS      4
#define FIXED_FONT    8
#define MAX_ATTRIBUTE 8

#define TEXT_FONT     1
#define GRAPHICS_FONT 3

#define FOREGROUND    0
#define BACKGROUND    1

#define GAME_RESTORE  0
#define GAME_SAVE     1
#define GAME_SCRIPT   2
#define GAME_RECORD   3
#define GAME_PLAYBACK 4
#define UNDO_SAVE     5
#define UNDO_RESTORE  6
#define GAME_SAVE_AUX 7
#define GAME_LOAD_AUX 8

#define MAX_TEXT_SIZE 8

/* Data access */


/* External data */

extern int GLOBALVER;
extern zbyte_t h_type;
extern zbyte_t h_config;
extern zword_t h_version;
extern zword_t h_data_size;
extern zword_t h_start_pc;
extern zword_t h_words_offset;
extern zword_t h_objects_offset;
extern zword_t h_globals_offset;
extern zword_t h_restart_size;
extern zword_t h_flags;
extern zword_t h_synonyms_offset;
extern zword_t h_file_size;
extern zword_t h_checksum;
extern zbyte_t h_interpreter;
extern zbyte_t h_interpreter_version;
extern zword_t h_alternate_alphabet_offset;
extern zword_t h_unicode_table;

extern int story_scaler;
extern int story_shift;
extern int property_mask;
extern int property_size_mask;

extern zword_t stack[STACK_SIZE];
extern zword_t sp;
extern zword_t fp;
extern zword_t frame_count;
extern unsigned long pc;
extern int interpreter_state;
extern int interpreter_status;

extern prog_char lookup_table[3][26] PROGMEM;

extern int screen_window;

/* Global routines */

/* configur.c */

void initialize_screen( void );
void restart_screen( void );
void configure( zbyte_t, zbyte_t );

/* control.c */

void z_check_arg_count( zword_t );
int  z_call( int, zword_t *, int );
void z_jump( zword_t );
void z_restart( void );
void restart_interp( int );
void z_ret( zword_t );
void z_catch( void );
void z_throw( zword_t, zword_t );


/* fileio.c */

void z_verify( void );
int z_restore( int, zword_t, zword_t, zword_t );
int z_save( int, zword_t, zword_t, zword_t );
void z_restore_undo( void );
void z_save_undo( void );
void z_open_playback( int );
void close_story( void );
unsigned int get_story_size( void );
void open_story( void );

/* input.c */

int get_line( zword_t, zword_t, zword_t );
void z_read_char( int, zword_t * );
void z_sread_aread( int, zword_t * );
void z_tokenise( int, zword_t * );
int input_character( int );
int input_line( int, unsigned long, int, int * );


/* interpre.c */

int interpret( void );


/* math.c */

void z_add( zword_t, zword_t );
void z_div( zword_t, zword_t );
void z_mul( zword_t, zword_t );
void z_sub( zword_t, zword_t );
void z_mod( zword_t, zword_t );
void z_or( zword_t, zword_t );
void z_and( zword_t, zword_t );
void z_not( zword_t );
void z_art_shift( zword_t, zword_t );
void z_log_shift( zword_t, zword_t );
void z_je( int, zword_t * );
void z_jg( zword_t, zword_t );
void z_jl( zword_t, zword_t );
void z_jz( zword_t );
void z_random( zword_t );
void z_test( zword_t, zword_t );


/* memory.c */

zbyte_t read_code_byte( void );
zbyte_t read_data_byte( unsigned long * );
zword_t read_code_word( void );
zword_t read_data_word( unsigned long * );
void write_data_byte( unsigned long * , zbyte_t);
void write_data_word( unsigned long * , zword_t);
zbyte_t get_byte(unsigned long);
zword_t get_word(unsigned long);
void set_byte(unsigned long, zbyte_t);
void set_word(unsigned long, zword_t);


/* object.c */

zword_t get_object_address( zword_t );
void z_insert_obj( zword_t, zword_t );
void z_remove_obj( zword_t );
void z_get_child( zword_t );
void z_get_sibling( zword_t );
void z_get_parent( zword_t );
void z_jin( zword_t, zword_t );
void z_clear_attr( zword_t, zword_t );
void z_set_attr( zword_t, zword_t );
void z_test_attr( zword_t, zword_t );


/* operand.c */

void z_piracy( int );
void z_store( int, zword_t );
void conditional_jump( int );
void store_operand( zword_t );
zword_t load_operand( int );
zword_t load_variable( int );


/* osdepend.c */

void fatal( void );


/* property.c */

void z_get_next_prop( zword_t, zword_t );
void z_get_prop( zword_t, zword_t );
void z_get_prop_addr( zword_t, zword_t );
void z_get_prop_len( zword_t );
void z_put_prop( zword_t, zword_t, zword_t );
void z_copy_table( zword_t, zword_t, zword_t );
void z_scan_table( int, zword_t * );
void z_loadb( zword_t, zword_t );
void z_loadw( zword_t, zword_t );
void z_storeb( zword_t, zword_t, zword_t );
void z_storew( zword_t, zword_t, zword_t );

/* screen.c */

void z_show_status( void );
void z_set_window( zword_t );
void z_print_table( int, zword_t * );


/* text.c */

void z_encode( zword_t, zword_t, zword_t, zword_t );
void z_new_line( void );
void z_print_char( zword_t );
void z_print_num( zword_t );
void z_print( void );
void z_print_addr( zword_t );
void z_print_paddr( zword_t );
void z_print_obj( zword_t );
void z_print_ret( void );
void z_buffer_mode( zword_t );
void z_output_stream( zword_t, zword_t );
void z_set_text_style( zword_t );
void decode_text( unsigned long * );
void encode_text( int, unsigned long, ZINT16 * );
void print_time( int, int );
void write_char( int );
void write_string( const char * );
void write_zchar( int );


/* variable.c */

void z_inc( zword_t );
void z_dec( zword_t );
void z_inc_chk( zword_t, zword_t );
void z_dec_chk( zword_t, zword_t );
void z_load( zword_t );
void z_pull( zword_t );
void z_push( zword_t );

#endif /* !defined(__ZTYPES_INCLUDED) */
