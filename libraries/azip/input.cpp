
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
  * input.c
  *
  * Input routines
  *
  */

#include "ztypes.h"

/* Statically defined word separator list */

static const char *separators = " \t\n\f.,?";
static zword_t dictionary_offset = 0;
static ZINT16 dictionary_size = 0;
static unsigned int entry_size = 0;

static void tokenise_line( zword_t, zword_t, zword_t, zword_t );
static unsigned long next_token( unsigned long, unsigned long, unsigned long *, int *, const char * );
static zword_t find_word( int, unsigned long, long );

/*
* z_read_char
*
* Read one character with optional timeout
*
*    argv[0] = input device (must be 1)
*    argv[1] = timeout value in tenths of a second (optional)
*    argv[2] = timeout action routine (optional)
*
*/

void z_read_char( int argc, zword_t * argv )
{
    int c;
    zword_t arg_list[2];

    /* Supply default parameters */

    if ( argc < 3 )
        argv[2] = 0;
    if ( argc < 2 )
        argv[1] = 0;

    /* If more than one characters was asked for then fail the call */

   if ( argv[0] != 0 && argv[0] != 1)
        c = 0;
    else
    {

        if ( 1 )
        {

            /* Setup the timeout routine argument list */

            arg_list[0] = argv[2];
            arg_list[1] = 0;       /* as per spec 1.0 */
            /* was: arg_list[1] = argv[1]/10; */

            /* Read a character with a timeout. If the input timed out then
            * call the timeout action routine. If the return status from the
            * timeout routine was 0 then try to read a character again */

            do
            {
                c = input_character( ( int ) argv[1] );
            }
            while ( c == -1 && z_call( 1, arg_list, ASYNC ) == 0 );

            /* Fail call if input timed out */

            if ( c == -1 )
                c = 0;
        }
    }

    store_operand( (zword_t)c );

}                               /* z_read_char */

/*
* z_sread_aread
*
* Read a line of input with optional timeout.
*
*    argv[0] = character buffer address
*    argv[1] = token buffer address
*    argv[2] = timeout value in seconds (optional)
*    argv[3] = timeout action routine (optional)
*
*/

void z_sread_aread( int argc, zword_t * argv )
{
   int terminator;

   /* Supply default parameters */

   if ( argc < 4 )
      argv[3] = 0;
   if ( argc < 3 )
      argv[2] = 0;
   if ( argc < 2 )
      argv[1] = 0;

   /* Refresh status line */

   //if ( h_type < V4 )
   //   z_show_status(  );

   /* Read the line then script and record it */

   terminator = get_line( argv[0], argv[2], argv[3] );

   /* Tokenise the line, if a token buffer is present */

   if ( argv[1] )
      tokenise_line( argv[0], argv[1], h_words_offset, 0 );

   /* Return the line terminator */

   if ( h_type > V4 )
      store_operand( ( zword_t ) terminator );

}                               /* z_sread_aread */

/*
* get_line
*
* Read a line of input and lower case it.
*
*/

int get_line( zword_t address, zword_t timeout, zword_t action_routine )
{
   unsigned long addr;
   int buflen, read_size, status, c;
   zword_t arg_list[2];

   addr = address;

   buflen = read_data_byte(&addr);

   /* Set read size and start of read buffer. The buffer may already be
    * primed with some text in V5 games. The Z-code will have already
    * displayed the text so we don't have to do that */

   if ( h_type > V4 )
   {
      read_size = read_data_byte(&addr);
   }
   else
   {
      read_size = 0;
   }

    /* Setup the timeout routine argument list */

    arg_list[0] = action_routine;
    arg_list[1] = 0;          /*  as per spec.1.0  */
    /* arg_list[1] = timeout/10; */

    /* Read a line with a timeout. If the input timed out then
    * call the timeout action routine. If the return status from the
    * timeout routine was 0 then try to read the line again */

    do
    {
        c = input_line( buflen, addr, timeout, &read_size );
        status = 0;
    }
    while ( c == -1 && ( status = z_call( 1, arg_list, ASYNC ) ) == 0 );

    /* Throw away any input if timeout returns success */

    if ( status )
        read_size = 0;

   /* Zero terminate line */

   if ( h_type > V4 )
   {
      set_byte(address + 1, read_size);
   }
   else
   {
      /* Zero terminate line (V1-4 only) */
      set_byte(address + read_size + 1, 0);
   }

   return ( c );

}                               /* get_line */


/*
* tokenise_line
*
* Convert a typed input line into tokens. The token buffer needs some
* additional explanation. The first byte is the maximum number of tokens
* allowed. The second byte is set to the actual number of token read. Each
* token is composed of 3 fields. The first (word) field contains the word
* offset in the dictionary, the second (byte) field contains the token length,
* and the third (byte) field contains the start offset of the token in the
* character buffer.
*
*/

static void tokenise_line( zword_t char_buf, zword_t token_buf, zword_t dictionary, zword_t flag )
{
    int i, count, words, token_length;
    long word_index, chop = 0;
    int slen;
    unsigned long str_end_addr;
    unsigned long cbuf_addr, tbuf_addr, tp_addr;
    unsigned long cp_addr = 0, token_addr = 0;
    char punctuation[16];
    zword_t word;

    /* Initialise character and token buffer pointers */
    tbuf_addr = token_buf;

    /* Find the string length */

    if ( h_type > V4 )
    {
        slen = ( unsigned char ) get_byte(char_buf + 1);
        str_end_addr = char_buf + 2 + slen;
    }
    else
    {
        for(cbuf_addr = char_buf + 1; get_byte(cbuf_addr) != '\0'; cbuf_addr++);
        slen = cbuf_addr - (char_buf + 1);
        str_end_addr = char_buf + 1 + slen;
    }

    /* Initialise word count and pointers */

    words = 0;
    cp_addr = ( h_type > V4 ) ? char_buf + 2 : char_buf + 1;
    tp_addr = token_buf + 2;

    /* Initialise dictionary */

    count = get_byte( dictionary++ );
    for ( i = 0; i < count; i++ )
    {
        punctuation[i] = get_byte( dictionary++ );
    }
    punctuation[i] = '\0';
    entry_size = get_byte( dictionary++ );
    dictionary_size = ( ZINT16 ) get_word( dictionary );
    dictionary_offset = dictionary + 2;

    /* Calculate the binary chop start position */

    if ( dictionary_size > 0 )
    {
        word_index = dictionary_size / 2;
        chop = 1;
        do
        chop *= 2;
        while ( word_index /= 2 );
    }

    /* Tokenise the line */

    do
    {
        /* Skip to next token */

        cp_addr = next_token( cp_addr, str_end_addr, &token_addr, &token_length, punctuation );
        if ( token_length )
        {

            /* If still space in token buffer then store word */

            if ( words <= get_byte(tbuf_addr) )
            {

                /* Get the word offset from the dictionary */

                word = find_word( token_length, token_addr, chop );

                /* Store the dictionary offset, token length and offset */

                if ( word || flag == 0 )
                {
                    set_byte(tp_addr, ( char ) ( word >> 8 ));
                    set_byte(tp_addr + 1, ( char ) ( word & 0xff ));
                }
                set_byte(tp_addr + 2, ( char ) token_length);
                set_byte(tp_addr + 3, ( char ) ( token_addr - char_buf ));

                /* Step to next token position and count the word */

                tp_addr += 4;
                words++;
            }
            else
            {

                /* Moan if token buffer space exhausted */

                //write_string( "Too many words typed, discarding: " );

            }
        }
    }
    while ( token_length );

    /* Store word count */

    set_byte(tbuf_addr + 1, ( char ) words);

}                               /* tokenise_line */

/*
* next_token
*
* Find next token in a string. The token (word) is delimited by a statically
* defined and a game specific set of word separators. The game specific set
* of separators look like real word separators, but the parser wants to know
* about them. An example would be: 'grue, take the axe. go north'. The
* parser wants to know about the comma and the period so that it can correctly
* parse the line. The 'interesting' word separators normally appear at the
* start of the dictionary, and are also put in a separate list in the game
* file.
*
*/

static unsigned long next_token( unsigned long s_addr, unsigned long str_end, unsigned long *token, int *length,
    const char *punctuation )
{
    int i;

    /* Set the token length to zero */

    *length = 0;

    /* Step through the string looking for separators */

    for ( ; s_addr < str_end; s_addr++ )
    {

        /* Look for game specific word separators first */

        for ( i = 0; punctuation[i] && get_byte(s_addr) != punctuation[i]; i++ )
            ;

        /* If a separator is found then return the information */

        if ( punctuation[i] )
        {

            /* If length has been set then just return the word position */

            if ( *length )
                return ( s_addr );
            else
            {

                /* End of word, so set length, token pointer and return string */

                ( *length )++;
                *token = s_addr;
                return ( ++s_addr );
            }
        }

        /* Look for statically defined separators last */

        for ( i = 0; separators[i] && get_byte(s_addr) != separators[i]; i++ )
            ;

        /* If a separator is found then return the information */

        if ( separators[i] )
        {

            /* If length has been set then just return the word position */

            if ( *length )
                return ( ++s_addr );
        }
        else
        {

            /* If first token character then remember its position */

            if ( *length == 0 )
                *token = s_addr;
            ( *length )++;
        }
    }

    return ( s_addr );

}                               /* next_token */

/*
* find_word
*
* Search the dictionary for a word. Just encode the word and binary chop the
* dictionary looking for it.
*
*/

static zword_t find_word( int len, unsigned long cp_addr, long chop )
{
    ZINT16 word[3];
    long word_index, offset, status;

    /* Don't look up the word if there are no dictionary entries */

    if ( dictionary_size == 0 )
        return ( 0 );

    /* Encode target word */

    encode_text( len, cp_addr, word );

    /* Do a binary chop search on the main dictionary, otherwise do
    * a linear search */

    word_index = chop - 1;

    if ( dictionary_size > 0 )
    {

        /* Binary chop until the word is found */

        while ( chop )
        {

            chop /= 2;

            /* Calculate dictionary offset */

            if ( word_index > ( dictionary_size - 1 ) )
                word_index = dictionary_size - 1;

            offset = dictionary_offset + ( word_index * entry_size );

            /* If word matches then return dictionary offset */

            if ( ( status = word[0] - ( ZINT16 ) get_word( offset + 0 ) ) == 0                &&
                ( status = word[1] - ( ZINT16 ) get_word( offset + 2 ) ) == 0                && 
                ( h_type < V4 || ( status = word[2] - ( ZINT16 ) get_word( offset + 4 ) ) == 0 ) )
                return ( ( zword_t ) offset );

            /* Set next position depending on direction of overshoot */

            if ( status > 0 )
            {
                word_index += chop;

                /* Deal with end of dictionary case */

                if ( word_index >= ( int ) dictionary_size )
                    word_index = dictionary_size - 1;
            }
            else
            {
                word_index -= chop;

                /* Deal with start of dictionary case */

                if ( word_index < 0 )
                    word_index = 0;
            }
        }
    }
    else
    {

        for ( word_index = 0; word_index < -dictionary_size; word_index++ )
        {

            /* Calculate dictionary offset */

            offset = dictionary_offset + ( word_index * entry_size );

            /* If word matches then return dictionary offset */

            if ( ( status = word[0] - ( ZINT16 ) get_word( offset + 0 ) ) == 0                &&
                ( status = word[1] - ( ZINT16 ) get_word( offset + 2 ) ) == 0                && 
                ( h_type < V4 || ( status = word[2] - ( ZINT16 ) get_word( offset + 4 ) ) == 0 ) )
                return ( ( zword_t ) offset );
        }
    }

    return ( 0 );

}                               /* find_word */

/*
* z_tokenise
*
*    argv[0] = character buffer address
*    argv[1] = token buffer address
*    argv[2] = alternate vocabulary table
*    argv[3] = ignore unknown words flag
*
*/

void z_tokenise( int argc, zword_t * argv )
{

    /* Supply default parameters */

    if ( argc < 4 )
        argv[3] = 0;
    if ( argc < 3 )
        argv[2] = h_words_offset;

    /* Convert the line to tokens */

    tokenise_line( argv[0], argv[1], argv[2], argv[3] );

}                               /* z_tokenise */

int input_character( int timeout )
{
    int c = getchar(  );

    /* Bureaucracy expects CR, not NL.  */
    return ( ( c == '\n' ) ? '\r' : c );
}                               /* input_character */

int input_line( int buflen, unsigned long addr, int timeout, int *read_size )
{
    int c = 0;

    *read_size = 0;

    do
    {
        if(Serial.available() > 0)
        {
            c = Serial.read();
            if ( (c != '\n') && (*read_size < buflen) )
            {
                ( *read_size )++;
                write_data_byte(&addr, tolower(c));
            }
        }
    }
    while ( c != '\n' );

    return c;
}                               /* input_line */
