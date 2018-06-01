
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
  * math.c
  *
  * Arithmetic, compare and logical instructions
  *
  */

#include "ztypes.h"

/*
* z_add
*
* Add two operands
*
*/

void z_add( zword_t a, zword_t b )
{
    store_operand( (zword_t)(a + b) );
}

/*
* z_sub
*
* Subtract two operands
*
*/

void z_sub( zword_t a, zword_t b )
{
    store_operand( (zword_t)(a - b) );
}

/*
* mul
*
* Multiply two operands
*
*/

void z_mul( zword_t a, zword_t b )
{
    store_operand( (zword_t)(( ZINT16 ) a * ( ZINT16 ) b) );
}

/*
* z_div
*
* Divide two operands
*
*/

void z_div( zword_t a, zword_t b )
{
    /* The magic rule is: round towards zero. */
    ZINT16 sa = ( ZINT16 ) a;
    ZINT16 sb = ( ZINT16 ) b;
    ZINT16 res;

    res = sa / sb;

    store_operand( res );
}

/*
* z_mod
*
* Modulus divide two operands
*
*/

void z_mod( zword_t a, zword_t b )
{
    /* The magic rule is: be consistent with divide, because
    * (a/b)*a + (a%b) == a. So (a%b) has the same sign as a. */
    ZINT16 sa = ( ZINT16 ) a;
    ZINT16 sb = ( ZINT16 ) b;
    ZINT16 res;

    if ( sb < 0 )
    {
        sb = -sb;
    }
    if ( sb == 0 )
    {
        res = 0;
    }
    if ( sa >= 0 )
    {
        res = sa % sb;
    }
    else
    {
        res = -( ( -sa ) % ( sb ) );
    }
    store_operand( res );

}

/*
* z_log_shift
*
* Shift +/- n bits
*
*/

void z_log_shift( zword_t a, zword_t b )
{

    if ( ( ZINT16 ) b > 0 )
        store_operand( (zword_t)( a << ( ZINT16 ) b ) );
    else
        store_operand( (zword_t)( a >> -( ( ZINT16 ) b ) ) );

}


/*
* z_art_shift
*
* Aritmetic shift +/- n bits
*
*/

void z_art_shift( zword_t a, zword_t b )
{
    if ( ( ZINT16 ) b > 0 )
        store_operand( ( zword_t ) ( ( ZINT16 ) a << ( ZINT16 ) b ) );
    else
        store_operand( ( zword_t ) ( ( ZINT16 ) a >> -( ( ZINT16 ) b ) ) );
}

/*
* z_or
*
* Logical OR
*
*/

void z_or( zword_t a, zword_t b )
{
    store_operand( (zword_t)(a | b) );
}

/*
* z_not
*
* Logical NOT
*
*/

void z_not( zword_t a )
{
    store_operand( (zword_t)(~a) );
}

/*
* z_and
*
* Logical AND
*
*/

void z_and( zword_t a, zword_t b )
{
    store_operand( (zword_t)(a & b) );
}

/*
* z_random
*
* Return random number between 1 and operand
*
*/

void z_random( zword_t a )
{
    if ( a == 0 )
        store_operand( 0 );
    else if ( a & 0x8000 )
    {                            /* (a < 0) - used to set seed with #RANDOM */
        SRANDOM_FUNC( ( unsigned int ) abs( a ) );
        store_operand( 0 );
    }
    else                         /* (a > 0) */
        store_operand( (zword_t)(( RANDOM_FUNC(  ) % a ) + 1) );
}

/*
* z_test
*
* Jump if operand 2 bit mask not set in operand 1
*
*/

void z_test( zword_t a, zword_t b )
{
    conditional_jump( ( ( ~a ) & b ) == 0 );
}

/*
* z_jz
*
* Compare operand against zero
*
*/

void z_jz( zword_t a )
{
    conditional_jump( a == 0 );
}

/*
* z_je
*
* Jump if operand 1 is equal to any other operand
*
*/

void z_je( int count, zword_t * operand )
{
    int i;

    for ( i = 1; i < count; i++ )
        if ( operand[0] == operand[i] )
        {
            conditional_jump( TRUE );
            return;
        }
        conditional_jump( FALSE );
}

/*
* z_jl
*
* Jump if operand 1 is less than operand 2
*
*/

void z_jl( zword_t a, zword_t b )
{
    conditional_jump( ( ZINT16 ) a < ( ZINT16 ) b );
}

/*
* z_jg
*
* Jump if operand 1 is greater than operand 2
*
*/

void z_jg( zword_t a, zword_t b )
{
    conditional_jump( ( ZINT16 ) a > ( ZINT16 ) b );
}
