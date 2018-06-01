
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
  * variable.c
  *
  * Variable manipulation routines
  *
  */

#include "ztypes.h"

/*
* z_load
*
* Load and store a variable value on stack.
*
*/

void z_load( zword_t variable )
{
    store_operand( load_variable( variable ) );
}                               /* load */

/*
* z_push
*
* Push a value onto the stack
*
*/

void z_push( zword_t value )
{
    stack[--sp] = value;
}                               /* push_var */

/*
* z_pull
*
* Pop a variable from the stack.
*
*/

void z_pull( zword_t variable )
{
    z_store( variable, stack[sp++] );
}                               /* pop_var */

/*
* z_inc
*
* Increment a variable.
*
*/

void z_inc( zword_t variable )
{
    z_store( variable, (zword_t)(load_variable( variable ) + 1) );
}                               /* increment */

/*
* z_dec
*
* Decrement a variable.
*
*/

void z_dec( zword_t variable )
{
    z_store( variable, (zword_t)(load_variable( variable ) - 1) );
}                               /* decrement */

/*
* z_inc_chk
*
* Increment a variable and then check its value against a target.
*
*/

void z_inc_chk( zword_t variable, zword_t target )
{
    ZINT16 value;

    value = ( ZINT16 ) load_variable( variable );
    z_store( variable, ++value );
    conditional_jump( value > ( ZINT16 ) target );
}                               /* increment_check */

/*
* z_dec_chk
*
* Decrement a variable and then check its value against a target.
*
*/

void z_dec_chk( zword_t variable, zword_t target )
{
    ZINT16 value;

    value = ( ZINT16 ) load_variable( variable );
    z_store( variable, --value );
    conditional_jump( value < ( ZINT16 ) target );
}                               /* decrement_check */
