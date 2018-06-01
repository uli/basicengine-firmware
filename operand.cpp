
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
  * operand.c
  *
  * Operand manipulation routines
  *
  */

#include "ztypes.h"

/*
* load_operand
*
* Load an operand, either: a variable, popped from the stack or a literal.
*
*/

zword_t load_operand( int type )
{
    zword_t operand;

    if ( type )
    {

        /* Type 1: byte literal, or type 2: operand specifier */

        operand = ( zword_t ) read_code_byte(  );
        if ( type == 2 )
        {

            /* If operand specifier non-zero then it's a variable, otherwise
            * it's the top of the stack */

            if ( operand )
            {
                return load_variable( operand );
            }
            else
            {
                return stack[sp++];
            }
        }
    }
    else
    {
        /* Type 0: word literal */
        return read_code_word(  );
    }
    return ( operand );

}                               /* load_operand */

/*
* store_operand
*
* Store an operand, either as a variable pushed on the stack.
*
*/

void store_operand( zword_t operand )
{
    zbyte_t specifier;

    /* Read operand specifier byte */

    specifier = read_code_byte(  );

    /* If operand specifier non-zero then it's a variable, otherwise it's the
    * pushed on the stack */

    if ( specifier )
        z_store( specifier, operand );
    else
        stack[--sp] = operand;

}                               /* store_operand */

/*
* load_variable
*
* Load a variable, either: a stack local variable, a global variable or
* the top of the stack.
*
*/

zword_t load_variable( int number )
{
    if ( number )
    {
        if ( number < 16 )
        {
            /* number in range 1 - 15, it's a stack local variable */
            return stack[fp - ( number - 1 )];
        }
        else
        {
            /* number > 15, it's a global variable */
            return get_word( h_globals_offset + ( ( number - 16 ) * 2 ) );
        }
    }
    else
    {
        /* number = 0, get from top of stack */
        return stack[sp];
    }

}                               /* load_variable */

/*
* z_store
*
* Store a variable, either: a stack local variable, a global variable or the
* top of the stack.
*
*/

void z_store( int number, zword_t variable )
{

    if ( number )
    {
        if ( number < 16 )

            /* number in range 1 - 15, it's a stack local variable */

            stack[fp - ( number - 1 )] = variable;
        else
            /* number > 15, it's a global variable */

            set_word( h_globals_offset + ( ( number - 16 ) * 2 ), variable );
    }
    else
        /* number = 0, get from top of stack */

        stack[sp] = variable;

}                               /* z_store */

/*
* z_piracy
*
* Supposed to jump if the game thinks that it is pirated.
*/
void z_piracy( int flag )
{
    conditional_jump( flag );
}

/*
* conditional_jump
*
* Take a jump after an instruction based on the flag, either true or false. The
* jump can be modified by the change logic flag. Normally jumps are taken
* when the flag is true. When the change logic flag is set then the jump is
* taken when flag is false. A PC relative jump can also be taken. This jump can
* either be a positive or negative byte or word range jump. An additional
* feature is the return option. If the jump offset is zero or one then that
* literal value is passed to the return instruction, instead of a jump being
* taken. Complicated or what!
*
*/

void conditional_jump( int flag )
{
    zbyte_t specifier;
    zword_t offset;

    /* Read the specifier byte */

    specifier = read_code_byte(  );

    /* If the reverse logic flag is set then reverse the flag */

    if ( specifier & 0x80 )
        flag = ( flag ) ? 0 : 1;

    /* Jump offset is in bottom 6 bits */

    offset = ( zword_t ) specifier & 0x3f;

    /* If the byte range jump flag is not set then load another offset byte */

    if ( ( specifier & 0x40 ) == 0 )
    {

        /* Add extra offset byte to existing shifted offset */

        offset = ( offset << 8 ) + read_code_byte(  );

        /* If top bit of offset is set then propogate the sign bit */

        if ( offset & 0x2000 )
            offset |= 0xc000;
    }

    /* If the flag is false then do the jump */

    if ( flag == 0 )
    {

        /* If offset equals 0 or 1 return that value instead */

        if ( offset == 0 || offset == 1 )
        {
            z_ret( offset );
        }
        else
        {                         /* Add offset to PC */
            pc = ( unsigned long ) ( pc + ( ZINT16 ) offset - 2 );
        }
    }
}                               /* conditional_jump */
