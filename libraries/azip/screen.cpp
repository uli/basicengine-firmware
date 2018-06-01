
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
  * screen.c
  *
  * Generic screen manipulation routines. Most of these routines call the machine
  * specific routines to do the actual work.
  *
  */

#include "ztypes.h"

/*
* z_set_window
*
* Put the cursor in the text or status window. The cursor is free to move in
* the status window, but is fixed to the input line in the text window.
*
*/

void z_set_window( zword_t w )
{
    screen_window = w;
}                               /* z_set_window */


/*
* z_show_status
*
* Format and output the status line for type 3 games only.
*
*/

void z_show_status( void )
{
    //write_string("\n\n[ ");

    /* Print the object description for global variable 16 */

    if ( load_variable( 16 ) != 0 )
        z_print_obj( load_variable( 16 ) );

    if ( get_byte( H_CONFIG ) & CONFIG_TIME )
    {

        /* If a time display print the hours and minutes from global
        * variables 17 and 18 */

        //write_string( " Time: " );
        print_time( load_variable( 17 ), load_variable( 18 ) );
    }
    else
    {

        /* If a moves/score display print the score and moves from global
        * variables 17 and 18 */

        //write_string( " Score: " );
        z_print_num( load_variable( 17 ) );

        //write_string( " Moves: " );
        z_print_num( load_variable( 18 ) );
    }

    //write_string(" ]\n\n> ");

}                               /* output_new_line */

/*
* z_print_table
*
* Writes text into a rectangular window on the screen.
*
*    argv[0] = start of text address
*    argv[1] = rectangle width
*    argv[2] = rectangle height (default = 1)
*
*/

void z_print_table( int argc, zword_t * argv )
{
    unsigned long address;
    unsigned int width, height;

    /* Supply default arguments */

    if ( argc < 3 )
        argv[2] = 1;

    /* Don't do anything if the window is zero high or wide */

    if ( argv[1] == 0 || argv[2] == 0 )
        return;

    address = argv[0];

    /* Write text in width * height rectangle */

    for ( height = 0; height < argv[2]; height++ )
    {

        for ( width = 0; width < argv[1]; width++ )
            write_char( read_data_byte( &address ) );
    }

}                               /* z_print_table */
