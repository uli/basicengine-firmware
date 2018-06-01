
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
  * text.c
  *
  * Text manipulation routines
  *
  */

#include "ztypes.h"


/*
* decode_text
*
* Convert ZSCII encoded text to ASCII. Text is encoded by squeezing each character
* into 5 bits. 3 x 5 bit encoded characters can fit in one word with a spare
* bit left over. The spare bit is used to signal to end of a string. The 5 bit
* encoded characters can either be actual character codes or prefix codes that
* modifier the following code.
*
*/

void decode_text( unsigned long *address )
{
    int i, synonym_flag, synonym = 0, zscii_flag, zscii = 0;
    int data, code, shift_state, shift_lock;
    unsigned long addr;

    /* Set state variables */

    shift_state = 0;
    shift_lock = 0;
    zscii_flag = 0;
    synonym_flag = 0;

    do
    {

        /*
        * Read one 16 bit word. Each word contains three 5 bit codes. If the
        * high bit is set then this is the last word in the string.
        */

        data = read_data_word( address );

        for ( i = 10; i >= 0; i -= 5 )
        {

            /* Get code, high bits first */

            code = ( data >> i ) & 0x1f;

            /* Synonym codes */

            if ( synonym_flag )
            {

                synonym_flag = 0;
                synonym = ( synonym - 1 ) * 64;
                addr = ( unsigned long ) get_word( h_synonyms_offset + synonym + ( code * 2 ) ) * 2;
                decode_text( &addr );
                shift_state = shift_lock;

            }
            /* ZSCII codes */
            else if ( zscii_flag )
            {

                /*
                * If this is the first part ZSCII ten-bit code then remember it.
                * Because the codes are only 5 bits you need two codes to make
                * one eight bit ASCII character. The first code contains the
                * top 5 bits (although only 3 bits are used at the moment).
                * The second code contains the bottom 5 bits.
                */

                if ( zscii_flag++ == 1 )
                {
                    zscii = code << 5;
                }
                /*
                * If this is the second part of a ten-bit ZSCII code then assemble the
                * character from the two codes and output it.
                */
                else
                {
                    zscii_flag = 0;
                    write_zchar( ( unsigned char ) ( zscii | code ) );
                }

            }

            /* Character codes */
            else if ( code > 5 )
            {

                code -= 6;

                /*
                * If this is character 0 in the punctuation set then the next two
                * codes make a ten-bit ZSCII character. (Std. Sec. 3.4)
                */

                if ( shift_state == 2 && code == 0 )
                {
                    zscii_flag = 1;
                }

                /*
                * If this is character 1 in the punctuation set then this
                * is a new line.
                */

                else if ( shift_state == 2 && code == 1 && h_type > V1 )
                {
                    z_new_line(  );
                }
                /*
                * This is a normal character so select it from the character
                * table appropriate for the current shift state.
                */

                else
                {
                    write_zchar( pgm_read_byte(&lookup_table[shift_state][code]) );
                }
                shift_state = shift_lock;

            }

            /* Special codes 0 to 5 */
            else
            {

                /* Space: 0     Output a space character.     */

                if ( code == 0 )
                {
                    write_zchar( ' ' );
                }
                else
                {
                    /* The use of the synonym and shift codes is the only 
                    * difference between the different versions.  
                    */

                    if ( h_type < V3 )
                    {

                        /* Newline or synonym: 1
                        * Output a newline character or set synonym flag.
                        */

                        if ( code == 1 )
                        {
                            if ( h_type == V1 )
                            {
                                z_new_line(  );
                            }
                            else
                            {
                                synonym_flag = 1;
                                synonym = code;
                            }
                        }
                        else
                        {
                            /*
                            * Shift keys: 2, 3, 4 or 5
                            *
                            * Shift keys 2 & 3 only shift the next character and can be used regardless of
                            * the state of the shift lock. Shift keys 4 & 5 lock the shift until reset.
                            *
                            * The following code implements the the shift code state transitions:
                            *               +-------------+-------------+-------------+-------------+
                            *               |       Shift   State       |        Lock   State       |
                            * +-------------+-------------+-------------+-------------+-------------+
                            * | Code        |      2      |       3     |      4      |      5      |
                            * +-------------+-------------+-------------+-------------+-------------+
                            * | lowercase   | uppercase   | punctuation | uppercase   | punctuation |
                            * | uppercase   | punctuation | lowercase   | punctuation | lowercase   |
                            * | punctuation | lowercase   | uppercase   | lowercase   | uppercase   |
                            * +-------------+-------------+-------------+-------------+-------------+
                            */
                            if ( code < 4 )
                            {
                                shift_state = ( shift_lock + code + 2 ) % 3;
                            }
                            else
                            {
                                shift_lock = shift_state = ( shift_lock + code ) % 3;
                            }
                        }

                    }
                    else /* not V3 */
                    {

                        /*
                        * Synonym table: 1, 2 or 3
                        *
                        * Selects which of three synonym tables the synonym
                        * code following in the next code is to use.
                        */
                        if ( code < 4 )
                        {
                            synonym_flag = 1;
                            synonym = code;
                        }
                        /*
                        * Shift key: 4 or 5
                        *
                        * Selects the shift state for the next character,
                        * either uppercase (4) or punctuation (5). The shift
                        * state automatically gets reset back to lowercase for
                        * V3+ games after the next character is output.
                        *
                        */
                        else
                        {
                            shift_state = code - 3;
                            shift_lock = 0;
                        }
                    }
                }
            }
        }
    }
    while ( ( data & 0x8000 ) == 0 );

}                               /* decode_text */

/*
* encode_text
*
* Pack a string into up to 9 codes or 3 words.
*
*/

void encode_text( int len, unsigned long s_addr, ZINT16 * buffer )
{
    int i, j, prev_table, table, next_table, shift_state, code, codes_count;
    char codes[9];

    /* Initialise codes count and prev_table number */

    codes_count = 0;
    prev_table = 0;

    /* Scan do the string one character at a time */

    while ( len-- )
    {

        /*
        * Set the table and code to be the ASCII character inducer, then
        * look for the character in the three lookup tables. If the
        * character isn't found then it will be an ASCII character.
        */

        table = 2;
        code = 0;
        for ( i = 0; i < 3; i++ )
        {
            for ( j = 0; j < 26; j++ )
            {
                if ( pgm_read_byte(&lookup_table[i][j]) == get_byte(s_addr) )
                {
                    table = i;
                    code = j;
                }
            }
        }

        /*
        * Type 1 and 2 games differ on how the shift keys are used. Switch
        * now depending on the game version.
        */

        if ( h_type < V3 )
        {

            /*
            * If the current table is the same as the previous table then
            * just store the character code, otherwise switch tables.
            */

            if ( table != prev_table )
            {

                /* Find the table for the next character */

                next_table = 0;
                if ( len )
                {
                    next_table = 2;
                    for ( i = 0; i < 3; i++ )
                    {
                        for ( j = 0; j < 26; j++ )
                        {
                            if ( pgm_read_byte(&lookup_table[i][j]) == get_byte(s_addr + 1) )
                                next_table = i;
                        }
                    }
                }

                /*
                * Calculate the shift key. This magic. See the description in
                * decode_text for more information on version 1 and 2 shift
                * key changes.
                */

                shift_state = ( table + ( prev_table * 2 ) ) % 3;

                /* Only store the shift key if there is a change in table */

                if ( shift_state )
                {

                    /*
                    * If the next character as the uses the same table as
                    * this character then change the shift from a single
                    * shift to a shift lock. Also remember the current
                    * table for the next iteration.
                    */

                    if ( next_table == table )
                    {
                        shift_state += 2;
                        prev_table = table;
                    }
                    else
                        prev_table = 0;

                    /* Store the code in the codes buffer */

                    if ( codes_count < 9 )
                        codes[codes_count++] = ( char ) ( shift_state + 1 );
                }
            }
        }
        else
        {

            /*
            * For V3 games each uppercase or punctuation table is preceded
            * by a separate shift key. If this is such a shift key then
            * put it in the codes buffer.
            */

            if ( table && codes_count < 9 )
                codes[codes_count++] = ( char ) ( table + 3 );
        }

        /* Put the character code in the code buffer */

        if ( codes_count < 9 )
            codes[codes_count++] = ( char ) ( code + 6 );

        /*
        * Cannot find character in table so treat it as a literal ASCII
        * code. The ASCII code inducer (code 0 in table 2) is followed by
        * the high 3 bits of the ASCII character followed by the low 5
        * bits to make 8 bits in total.
        */

        if ( table == 2 && code == 0 )
        {
            if ( codes_count < 9 )
                codes[codes_count++] = ( char ) ( ( get_byte(s_addr) >> 5 ) & 0x07 );
            if ( codes_count < 9 )
                codes[codes_count++] = ( char ) ( get_byte(s_addr) & 0x1f );
        }

        /* Advance to next character */

        s_addr++;

    }

    /* Pad out codes with shift 5's */

    while ( codes_count < 9 )
        codes[codes_count++] = 5;

    /* Pack codes into buffer */

    buffer[0] = ( ( ZINT16 ) codes[0] << 10 ) | ( ( ZINT16 ) codes[1] << 5 ) | ( ZINT16 ) codes[2];
    buffer[1] = ( ( ZINT16 ) codes[3] << 10 ) | ( ( ZINT16 ) codes[4] << 5 ) | ( ZINT16 ) codes[5];
    buffer[2] = ( ( ZINT16 ) codes[6] << 10 ) | ( ( ZINT16 ) codes[7] << 5 ) | ( ZINT16 ) codes[8];

    /* Terminate buffer at 6 or 9 codes depending on the version */

    if ( h_type < V4 )
        buffer[1] |= 0x8000;
    else
        buffer[2] |= 0x8000;

}                               /* encode_text */

/*
* write_zchar
*
* High level Z-code character output routine. Translates Z-code characters to
* machine specific character(s) before output. If it cannot translate it then
* use the default translation. If the character is still unknown then display
* a '?'.
*
*/

void write_zchar( int c )
{
    char xlat_buffer[MAX_TEXT_SIZE + 1];
    int i;

    c = ( unsigned int ) ( c & 0xff );

    /* If character is not special character then just write it */

    if ( c >= ' ' && c <= '~' )
    {
        write_char( c );
    }
    else if ( c == 13 )
    {
        write_char( '\r' );
    }
    else
    {
        /* Put default character in translation buffer */
        xlat_buffer[0] = '?';
        xlat_buffer[1] = '\0';

        /* If translation fails then supply a default */
        if ( 1 )
        {
            if ( c > 23 && c < 28 )
            {
                /* Arrow keys - these must the keyboard keys used for input */
                static char xlat[4] = { '\\', '/', '+', '-' };

                xlat_buffer[0] = xlat[c - 24];
                xlat_buffer[1] = '\0';
            }
            else if ( c == 0 )
            {
                /* Null - print nothing */
                xlat_buffer[0] = '\0';
            }
            else if ( c < 32 )
            {
                /* Some other control character: print an octal escape. */
                xlat_buffer[0] = '\\';
                xlat_buffer[1] = ( char ) ( '0' + ( ( c >> 6 ) & 7 ) );
                xlat_buffer[2] = ( char ) ( '0' + ( ( c >> 3 ) & 7 ) );
                xlat_buffer[3] = ( char ) ( '0' + ( c & 7 ) );
                xlat_buffer[4] = '\0';
            }
            else if ( c > 178 && c < 219 )
            {
                /* IBM line drawing characters to ASCII characters */
                if ( c == 179 )
                    xlat_buffer[0] = '|';
                else if ( c == 186 )
                    xlat_buffer[0] = '#';
                else if ( c == 196 )
                    xlat_buffer[0] = '-';
                else if ( c == 205 )
                    xlat_buffer[0] = '=';
                else
                    xlat_buffer[0] = '+';
                xlat_buffer[1] = '\0';
            }
            else if ( c > 154 && c < 164 )
            {
                /* German character replacements */
                static char xlat[] = "aeoeueAeOeUess>><<";

                xlat_buffer[0] = xlat[( ( c - 155 ) * 2 ) + 0];
                xlat_buffer[1] = xlat[( ( c - 155 ) * 2 ) + 1];
                xlat_buffer[2] = '\0';
            }
        }

        /* Substitute translated characters */

        for ( i = 0; xlat_buffer[i] != '\0'; i++ )
        {
            write_char( ( unsigned char ) xlat_buffer[i] );
        }

    }
}                               /* write_zchar */


/*
* write_char
*
* High level character output routine. The write_char routine is slightly
* complicated by the fact that the output can be limited by a fixed character
* count, as well as, filling up the buffer.
*
*/
void write_char( int c )
{
    static unsigned char line_count = 0;
    
    if ( screen_window == TEXT_WINDOW )
    {
         if(c == '\n')
             line_count = 0;
         else
         {
             line_count++;
             if(line_count >= 80)
             {
                 line_count = 1;
                 Serial.write('\n');
             }
         }
         
         Serial.write(c);
    }
}                               /* write_char */


/*
* write_string
*
* Output a string
*
*/

void write_string( const prog_char *s )
{
    while ( *s )
        write_char( *s++ );

}                               /* write_string */



/*
* z_output_stream
*
* Set various printing modes. These can be: disabling output, scripting and
* redirecting output. Redirection is peculiar. I use it to format the status
* line for V1 to V3 games, otherwise it wasn't used. V4 games format the
* status line themselves in an internal buffer in the writeable data area.
* To use the normal text decoding routines they have to redirect output to
* the writeable data area. This is done by passing in a buffer pointer.
* The first word of the buffer will receive the number of characters
* written since the output was redirected. The remainder of the buffer
* will contain the redirected text.
*
*/

void z_output_stream( zword_t type, zword_t option )
{
    if( ( ZINT16 )type == 3)
    {
        screen_window = STATUS_WINDOW;
    }
    else if( ( ZINT16 )type == -3)
    {
        screen_window = TEXT_WINDOW;
    }
}                               /* z_output_stream */

/*
* z_print_char
*
* Write a character.
*
*/

void z_print_char( zword_t c )
{
    write_zchar( ( char ) c );
}                               /* z_print_char */

/*
* z_print_num
*
* Write a signed number.
*
*/

void z_print_num( zword_t num )
{
    int i, count;
    char buffer[10];

    i = ( ZINT16 ) num;
    sprintf( buffer, "%d", i );
    count = strlen( buffer );
    for ( i = 0; i < count; i++ )
        write_char( buffer[i] );

}                               /* z_print_num */

/*
* z_print_paddr
*
* Print using a packed address. Packed addresses are used to save space and
* reference addresses outside of the data region.
*
*/

void z_print_paddr( zword_t packed_address )
{
    unsigned long address;

    /* Convert packed address to real address */
    address = ( unsigned long ) packed_address * story_scaler;

    /* Decode and output text at address */
    decode_text( &address );

}                               /* z_print_paddr */

/*
* z_print_addr
*
* Print using a real address. Real addresses are just offsets into the
* data region.
*
*/

void z_print_addr( zword_t offset )
{
    unsigned long address;

    address = offset;

    /* Decode and output text at address */
    decode_text( &address );

}                               /* z_print_addr */

/*
* z_print_obj
*
* Print an object description. Object descriptions are stored as ASCII
* strings at the front of the property list for the object.
*
*/

void z_print_obj( zword_t obj )
{
    zword_t offset;
    unsigned long address;

    /* Check for NULL object */
    if ( obj == 0 )
        return;

    /* Calculate address of property list */
    offset = get_object_address( obj );
    offset += ( h_type < V4 ) ? O3_PROPERTY_OFFSET : O4_PROPERTY_OFFSET;

    /* Read the property list address and skip the count byte */
    address = ( unsigned long ) get_word( offset ) + 1;

    /* Decode and output text at address */
    decode_text( &address );

}                               /* z_print_obj */

/*
* z_print
*
* Print the string embedded in the instruction stream at this point. All
* strings that do not need to be referenced by address are embedded in the
* instruction stream. All strings that can be refered to by address are placed
* at the end of the code region and referenced by packed address.
*
*/

void z_print( void )
{

    /* Decode and output text at PC */
    decode_text( &pc );

}                               /* z_print */

/*
* z_print_ret
*
* Print a string embedded in the instruction stream as with print_literal,
* except flush the output buffer and write a new line. After this return from
* the current subroutine with a status of true.
*
*/

void z_print_ret( void )
{

    z_print(  );
    z_new_line(  );
    z_ret( TRUE );

}                               /* z_print_ret */

/*
* z_new_line
*
* Simply flush the current contents of the output buffer followed by a new
* line.
*
*/

void z_new_line( void )
{
    write_char('\n');
}                               /* z_new_line */

/*
* print_time
*
* Print the time as HH:MM [am|pm]. This is a bit language dependent and can
* quite easily be changed. If you change the size of the time string output
* then adjust the status line position in display_status_line.
*
*/

void print_time( int hours, int minutes )
{
    int pm_indicator;

    /* Remember if time is pm */
    pm_indicator = ( hours < 12 ) ? OFF : ON;

    /* Convert 24 hour clock to 12 hour clock */
    hours %= 12;
    if ( hours == 0 )
        hours = 12;

    /* Write hour right justified */
    if ( hours < 10 )
        write_char( ' ' );
    z_print_num( (zword_t)hours );

    /* Write hours/minutes separator */
    write_char( ':' );

    /* Write minutes zero filled */
    if ( minutes < 10 )
        write_char( '0' );
    z_print_num( (zword_t)minutes );

    /* Write the am or pm string */
    if ( pm_indicator == ON )
        write_string( " pm" );
    else
        write_string( " am" );

}                               /* print_time */

/*
* z_encode
*
* Convert text to packed text.
*
*/

void z_encode( zword_t word_addr, zword_t word_length, zword_t word_offset, zword_t dest_addr )
{
    unsigned long addr;
    int i;
    ZINT16 word[3];

    addr = word_addr + word_offset;

    /* Encode the word */

    encode_text( word_length, addr, word );

    /* Move the encoded word, byte swapped, into the destination buffer */

    for ( i = 0; i < 3; i++, dest_addr += 2 )
        set_word( dest_addr, word[i] );

}                               /* z_encode */
