
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
  * configure
  *
  * Initialise global and type specific variables.
  *
  */
  
#include "ztypes.h"

void configure( zbyte_t min_version, zbyte_t max_version )
{

    h_type = get_byte( H_TYPE );

    GLOBALVER = h_type;

    if ( h_type < min_version || h_type > max_version ||
       ( get_byte( H_CONFIG ) & CONFIG_BYTE_SWAPPED ) )
    {
        fatal();
    }

    if ( h_type < V4 )
    {
        story_scaler = 2;
        story_shift = 1;
        property_mask = P3_MAX_PROPERTIES - 1;
        property_size_mask = 0xe0;
    }
    else if ( h_type < V8 )
    {
        story_scaler = 4;
        story_shift = 2;
        property_mask = P4_MAX_PROPERTIES - 1;
        property_size_mask = 0x3f;
    }
    else
    {
        story_scaler = 8;
        story_shift = 3;
        property_mask = P4_MAX_PROPERTIES - 1;
        property_size_mask = 0x3f;
    }

    h_config = get_byte( H_CONFIG );
    h_version = get_word( H_VERSION );
    h_data_size = get_word( H_DATA_SIZE );
    h_start_pc = get_word( H_START_PC );
    h_words_offset = get_word( H_WORDS_OFFSET );
    h_objects_offset = get_word( H_OBJECTS_OFFSET );
    h_globals_offset = get_word( H_GLOBALS_OFFSET );
    h_restart_size = get_word( H_RESTART_SIZE );
    h_flags = get_word( H_FLAGS );
    h_synonyms_offset = get_word( H_SYNONYMS_OFFSET );
    h_file_size = get_word( H_FILE_SIZE );
    if ( h_file_size == 0 )
        h_file_size = get_story_size(  );
    h_checksum = get_word( H_CHECKSUM );
    h_alternate_alphabet_offset = get_word( H_ALTERNATE_ALPHABET_OFFSET );

    if ( h_type >= V5 )
    {
        h_unicode_table = get_word( H_UNICODE_TABLE );
    }

}                               /* configure */

void initialize_screen( void )
{
    h_interpreter = INTERP_MSDOS;
}                               /* initialize_screen */

void restart_screen( void )
{
    zbyte_t high = 1, low = 0;

    set_byte( H_STANDARD_HIGH, high );
    set_byte( H_STANDARD_LOW, low );

    if ( h_type < V4 )
        set_byte( H_CONFIG, ( get_byte( H_CONFIG ) | CONFIG_NOSTATUSLINE ) );

    /* Force graphics off as we can't do them */
    set_word( H_FLAGS, ( get_word( H_FLAGS ) & ( ~GRAPHICS_FLAG ) ) );

}                               /* restart_screen */
