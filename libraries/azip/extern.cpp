
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
  * extern.c
  *
  * Global data.
  *
  */

#include "azip.h"
#include "ztypes.h"

AZIP::AZIP()
{
  h_type = 0;
  h_config = 0;
  h_version = 0;
  h_data_size = 0;
  h_start_pc = 0;
  h_words_offset = 0;
  h_objects_offset = 0;
  h_globals_offset = 0;
  h_restart_size = 0;
  h_flags = 0;
  h_synonyms_offset = 0;
  h_file_size = 0;
  h_checksum = 0;
  h_interpreter = INTERP_MSDOS;
  h_interpreter_version = 'B'; /* Interpreter version 2 */
  h_alternate_alphabet_offset = 0;
  h_unicode_table = 0;

  /* Game version specific data */

  story_scaler = 0;
  story_shift = 0;
  property_mask = 0;
  property_size_mask = 0;
  frame_count = 0;        /* frame pointer for get_fp */
  pc = 0;
  interpreter_state = RUN;
  interpreter_status = 0;
  screen_window = TEXT_WINDOW;

  halt = FALSE;

  dictionary_offset = 0;
  dictionary_size = 0;
  entry_size = 0;
  
  save_name[0] = 0;
  auxilary_name[0] = 0;
}

/* Character translation tables */

char lookup_table[3][26] PROGMEM = {
    {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'},
    {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'},
    {' ','\n','0','1','2','3','4','5','6','7','8','9','.',',','!','?','_','#','\'','\"','/','\\','-',':','(',')'}
};

