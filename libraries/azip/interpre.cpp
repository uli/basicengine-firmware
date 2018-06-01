
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
  * interpre.c
  *
  * Main interpreter loop
  *
  */

#include "ztypes.h"

//#define DEBUG_TERPRE

static int halt = FALSE;

/*
* interpret
*
* Interpret Z code
*
*/

int interpret(  )
{
    zbyte_t opcode;
    zword_t specifier, operand[8];
    int maxoperands, count, extended, i;

    interpreter_status = 1;

    /* Loop until HALT instruction executed */

    for ( interpreter_state = RUN; interpreter_state == RUN && halt == FALSE; )
    {

        /* Load opcode and set operand count */

        opcode = read_code_byte(  );
        if ( h_type > V4 && opcode == 0xbe )
        {
            opcode = read_code_byte(  );
            extended = TRUE;
        }
        else
            extended = FALSE;
        count = 0;

        /* Multiple operand instructions */

        if ( ( opcode < 0x80 || opcode > 0xc0 ) || extended == TRUE )
        {

            /* Two operand class, load both operands */

            if ( opcode < 0x80 && extended == FALSE )
            {
                operand[count++] = load_operand( ( opcode & 0x40 ) ? 2 : 1 );
                operand[count++] = load_operand( ( opcode & 0x20 ) ? 2 : 1 );
                opcode &= 0x1f;
            }
            else
            {

                /* Variable operand class, load operand specifier */

                opcode &= 0x3f;
                if ( opcode == 0x2c || opcode == 0x3a )
                {                   /* Extended CALL instruction */
                    specifier = read_code_word(  );
                    maxoperands = 8;
                }
                else
                {
                    specifier = read_code_byte(  );
                    maxoperands = 4;
                }

                /* Load operands */

                for ( i = ( maxoperands - 1 ) * 2; i >= 0; i -= 2 )
                    if ( ( ( specifier >> i ) & 0x03 ) != 3 )
                        operand[count++] = load_operand( ( specifier >> i ) & 0x03 );
                    else
                        i = 0;
            }

            if ( extended == TRUE )
            {
#ifdef DEBUG_TERPRE
                fprintf( stderr, "PC = 0x%08lx   Op%s = 0x%02x   %d, %d, %d\n", pc, "(EX)", opcode,
                    operand[0], operand[1], operand[2] );
#endif
                switch ( ( char ) opcode )
                {

                    /* Extended operand instructions */

                case 0x00:
                    z_save( count, operand[0], operand[1], operand[2] );
                    break;
                case 0x01:
                    z_restore( count, operand[0], operand[1], operand[2] );
                    break;
                case 0x02:
                    z_log_shift( operand[0], operand[1] );
                    break;
                case 0x03:
                    z_art_shift( operand[0], operand[1] );
                    break;
                case 0x04:
                    //z_set_font( operand[0] );
                    break;

                case 0x09:
                    z_save_undo(  );
                    break;
                case 0x0a:
                    z_restore_undo(  );
                    break;

                default:
                    fatal();
                }
            }
            else
            {
#ifdef DEBUG_TERPRE
                fprintf( stderr, "PC = 0x%08lx   Op%s = 0x%02x   %d, %d, %d\n", pc, "(2+)", opcode,
                    operand[0], operand[1], operand[2] );
#endif
                switch ( ( char ) opcode )
                {

                    /* Two or multiple operand instructions */
                case 0x01:
                    z_je( count, operand );
                    break;
                case 0x02:
                    z_jl( operand[0], operand[1] );
                    break;
                case 0x03:
                    z_jg( operand[0], operand[1] );
                    break;
                case 0x04:
                    z_dec_chk( operand[0], operand[1] );
                    break;
                case 0x05:
                    z_inc_chk( operand[0], operand[1] );
                    break;
                case 0x06:
                    z_jin( operand[0], operand[1] );
                    break;
                case 0x07:
                    z_test( operand[0], operand[1] );
                    break;
                case 0x08:
                    z_or( operand[0], operand[1] );
                    break;
                case 0x09:
                    z_and( operand[0], operand[1] );
                    break;
                case 0x0a:
                    z_test_attr( operand[0], operand[1] );
                    break;
                case 0x0b:
                    z_set_attr( operand[0], operand[1] );
                    break;
                case 0x0c:
                    z_clear_attr( operand[0], operand[1] );
                    break;
                case 0x0d:
                    z_store( operand[0], operand[1] );
                    break;
                case 0x0e:
                    z_insert_obj( operand[0], operand[1] );
                    break;
                case 0x0f:
                    z_loadw( operand[0], operand[1] );
                    break;
                case 0x10:
                    z_loadb( operand[0], operand[1] );
                    break;
                case 0x11:
                    z_get_prop( operand[0], operand[1] );
                    break;
                case 0x12:
                    z_get_prop_addr( operand[0], operand[1] );
                    break;
                case 0x13:
                    z_get_next_prop( operand[0], operand[1] );
                    break;
                case 0x14:
                    z_add( operand[0], operand[1] );
                    break;
                case 0x15:
                    z_sub( operand[0], operand[1] );
                    break;
                case 0x16:
                    z_mul( operand[0], operand[1] );
                    break;
                case 0x17:
                    z_div( operand[0], operand[1] );
                    break;
                case 0x18:
                    z_mod( operand[0], operand[1] );
                    break;
                case 0x19:
                    z_call( count, operand, FUNCTION );
                    break;
                case 0x1a:
                    z_call( count, operand, PROCEDURE );
                    break;
                case 0x1b:
                    //z_set_colour( operand[0], operand[1] );
                    break;
                case 0x1c:
                    z_throw( operand[0], operand[1] );
                    break;

                    /* Multiple operand instructions */

                case 0x20:
                    z_call( count, operand, FUNCTION );
                    break;
                case 0x21:
                    z_storew( operand[0], operand[1], operand[2] );
                    break;
                case 0x22:
                    z_storeb( operand[0], operand[1], operand[2] );
                    break;
                case 0x23:
                    z_put_prop( operand[0], operand[1], operand[2] );
                    break;
                case 0x24:
                    //pc -= 4;
                    z_sread_aread( count, operand );
                    break;
                case 0x25:
                    z_print_char( operand[0] );
                    break;
                case 0x26:
                    z_print_num( operand[0] );
                    break;
                case 0x27:
                    z_random( operand[0] );
                    break;
                case 0x28:
                    z_push( operand[0] );
                    break;
                case 0x29:
                    z_pull( operand[0] );
                    break;
                case 0x2a:
                    //z_split_window( operand[0] );
                    break;
                case 0x2b:
                    z_set_window( operand[0] );
                    break;
                case 0x2c:
                    z_call( count, operand, FUNCTION );
                    break;
                case 0x2d:
                    //z_erase_window( operand[0] );
                    break;
                case 0x2e:
                    //z_erase_line( operand[0] );
                    break;
                case 0x2f:
                    //z_set_cursor( operand[0], operand[1] );
                    break;

                case 0x31:
                    //z_set_text_style( operand[0] );
                    break;
                case 0x32:
                    //z_buffer_mode( operand[0] );
                    break;
                case 0x33:
                    z_output_stream( operand[0], operand[1] );
                    break;
                case 0x34:
                    //z_input_stream( operand[0] );
                    break;
                case 0x35:
                    //sound( count, operand );
                    break;
                case 0x36:
                    z_read_char( count, operand );
                    break;
                case 0x37:
                    z_scan_table( count, operand );
                    break;
                case 0x38:
                    z_not( operand[0] );
                    break;
                case 0x39:
                    z_call( count, operand, PROCEDURE );
                    break;
                case 0x3a:
                    z_call( count, operand, PROCEDURE );
                    break;
                case 0x3b:
                    z_tokenise( count, operand );
                    break;
                case 0x3c:
                    z_encode( operand[0], operand[1], operand[2], operand[3] );
                    break;
                case 0x3d:
                    z_copy_table( operand[0], operand[1], operand[2] );
                    break;
                case 0x3e:
                    z_print_table( count, operand );
                    break;
                case 0x3f:
                    z_check_arg_count( operand[0] );
                    break;

                default:
                    fatal();
                }
            }
        }
        else
        {

            /* Single operand class, load operand and execute instruction */

            if ( opcode < 0xb0 )
            {
                operand[0] = load_operand( ( opcode >> 4 ) & 0x03 );
#ifdef DEBUG_TERPRE
                fprintf( stderr, "PC = 0x%08lx   Op%s = 0x%02x   %d\n", pc, "(1 )", opcode,
                    operand[0] );
#endif
                switch ( ( char ) opcode & 0x0f )
                {
                case 0x00:
                    z_jz( operand[0] );
                    break;
                case 0x01:
                    z_get_sibling( operand[0] );
                    break;
                case 0x02:
                    z_get_child( operand[0] );
                    break;
                case 0x03:
                    z_get_parent( operand[0] );
                    break;
                case 0x04:
                    z_get_prop_len( operand[0] );
                    break;
                case 0x05:
                    z_inc( operand[0] );
                    break;
                case 0x06:
                    z_dec( operand[0] );
                    break;
                case 0x07:
                    z_print_addr( operand[0] );
                    break;
                case 0x08:
                    z_call( 1, operand, FUNCTION );
                    break;
                case 0x09:
                    z_remove_obj( operand[0] );
                    break;
                case 0x0a:
                    z_print_obj( operand[0] );
                    break;
                case 0x0b:
                    z_ret( operand[0] );
                    break;
                case 0x0c:
                    z_jump( operand[0] );
                    break;
                case 0x0d:
                    z_print_paddr( operand[0] );
                    break;
                case 0x0e:
                    z_load( operand[0] );
                    break;
                case 0x0f:
                    if ( h_type > V4 )
                        z_call( 1, operand, PROCEDURE );
                    else
                        z_not( operand[0] );
                    break;
                }
            }
            else
            {

                /* Zero operand class, execute instruction */
#ifdef DEBUG_TERPRE
                fprintf( stderr, "PC = 0x%08lx   Op%s = 0x%02x\n", pc, "(0 )", opcode );
#endif
                switch ( ( char ) opcode & 0x0f )
                {
                case 0x00:
                    z_ret( TRUE );
                    break;
                case 0x01:
                    z_ret( FALSE );
                    break;
                case 0x02:
                    z_print(  );
                    break;
                case 0x03:
                    z_print_ret(  );
                    break;
                case 0x04:
                    /* z_nop */
                    break;
                case 0x05:
                    z_save( 0, 0, 0, 0 );
                    break;
                case 0x06:
                    z_restore( 0, 0, 0, 0 );
                    break;
                case 0x07:
                    z_restart(  );
                    break;
                case 0x08:
                    z_ret( stack[sp++] );
                    break;
                case 0x09:
                    z_catch(  );
                    break;
                case 0x0a:
                    halt = TRUE;  /* z_quit */
                    break;
                case 0x0b:
                    z_new_line(  );
                    break;
                case 0x0c:
                    z_show_status(  );
                    break;
                case 0x0d:
                    z_verify(  );
                    break;

                case 0x0f:
                    z_piracy( TRUE );
                    break;

                default:
                    fatal();
                }
            }
        }
    }

    return ( interpreter_status );

}                               /* interpret */
