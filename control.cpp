
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
  * control.c
  *
  * Functions that alter the flow of control.
  *
  */

#include "ztypes.h"

/*
 * z_check_arg_count
 *
 * Jump if argument is present.
 *
 */

void z_check_arg_count( zword_t argc )
{

    conditional_jump( argc <= ( zword_t ) ( stack[fp + 1] & ARGS_MASK ) );

}                               /* z_check_arg_count */

/*
 * z_call
 *
 * Call a subroutine. Save PC and FP then load new PC and initialise stack based
 * local arguments.
 *
 * Implements: call_1s, call_1n, call_2s, call_2n, call, call_vs, call_vs2, call_vn, call_vn2
 *
 */

int z_call( int argc, zword_t * argv, int type )
{
    zword_t arg;
    int i = 1, args, status = 0;

    /* Convert calls to 0 as returning FALSE */

    if ( argv[0] == 0 )
    {
        if ( type == FUNCTION )
            store_operand( FALSE );
        return ( 0 );
    }

    /* Save current PC, FP and argument count on stack */

    stack[--sp] = ( zword_t ) ( pc >> 16 );
    stack[--sp] = ( zword_t ) ( pc & 0xFFFF );
    stack[--sp] = fp;
    stack[--sp] = ( argc - 1 ) | type;

    /* Create FP for new subroutine and load new PC */

    fp = sp - 1;
    pc = ( unsigned long ) argv[0] * story_scaler;

    /* Read argument count and initialise local variables */

    args = ( unsigned int ) read_code_byte(  );

    while ( --args >= 0 )
    {
        arg = ( h_type > V4 ) ? 0 : read_code_word(  );
        stack[--sp] = ( --argc > 0 ) ? argv[i++] : arg;
    }

    /* If the call is asynchronous then call the interpreter directly.
     * We will return back here when the corresponding return frame is
     * encountered in the ret call. */

    if ( type == ASYNC )
    {
        status = interpret(  );
        interpreter_state = RUN;
        interpreter_status = 1;
    }

    return ( status );

}                               /* z_call */

/*
 * z_ret
 *
 * Return from subroutine. Restore FP and PC from stack.
 *
 */

void z_ret( zword_t value )
{
    zword_t argc;

    /* Clean stack */

    sp = fp + 1;

    /* Restore argument count, FP and PC */

    argc = stack[sp++];
    fp = stack[sp++];
    pc = stack[sp++];
    pc += ( unsigned long ) stack[sp++] << 16;

    /* If this was an async call then stop the interpreter and return
     * the value from the async routine. This is slightly hacky using
     * a global state variable, but ret can be called with conditional_jump
     * which in turn can be called from all over the place, sigh. A
     * better design would have all opcodes returning the status RUN, but
     * this is too much work and makes the interpreter loop look ugly */

    if ( ( argc & TYPE_MASK ) == ASYNC )
    {
        interpreter_state = STOP;
        interpreter_status = ( int ) value;
    }
    else
    {
        /* Return subroutine value for function call only */
        if ( ( argc & TYPE_MASK ) == FUNCTION )
        {
            store_operand( value );
        }
    }
}                               /* z_ret */

/*
 * z_jump
 *
 * Unconditional jump. Jump is PC relative.
 *
 */

void z_jump( zword_t offset )
{

    pc = ( unsigned long ) ( pc + ( ZINT16 ) offset - 2 );

}                               /* z_jump */

/*
 * z_restart
 *
 * Restart game by initialising environment and reloading start PC.
 *
 */

void z_restart( void )
{
    unsigned int scripting_flag;

    /* Randomise */

    SRANDOM_FUNC( ( unsigned int ) millis( ) );

    /* Remember scripting state */

    scripting_flag = get_word( H_FLAGS ) & SCRIPTING_FLAG;

    /* Restart the screen */

    restart_screen(  );

    /* Reset the interpreter state */

    restart_interp( scripting_flag );

    /* Load start PC, SP and FP */

    pc = h_start_pc;
    sp = STACK_SIZE;
    fp = STACK_SIZE - 1;

}                               /* z_restart */


/*
 * restart_interp
 *
 * Do all the things which need to be done after startup, restart, and restore
 * commands.
 *
 */

void restart_interp( int scripting_flag )
{
    if ( scripting_flag )
        set_word( H_FLAGS, ( get_word( H_FLAGS ) | SCRIPTING_FLAG ) );

    set_byte( H_INTERPRETER, h_interpreter );
    set_byte( H_INTERPRETER_VERSION, h_interpreter_version );
    set_byte( H_SCREEN_ROWS, DEFAULT_ROWS ); /* Screen dimension in characters */
    set_byte( H_SCREEN_COLUMNS, DEFAULT_COLS );

    set_byte( H_SCREEN_LEFT, 0 ); /* Screen dimension in smallest addressable units, ie. pixels */
    set_byte( H_SCREEN_RIGHT, DEFAULT_COLS );
    set_byte( H_SCREEN_TOP, 0 );
    set_byte( H_SCREEN_BOTTOM, DEFAULT_ROWS );

    set_byte( H_MAX_CHAR_WIDTH, 1 ); /* Size of a character in screen units */
    set_byte( H_MAX_CHAR_HEIGHT, 1 );

    /* Initialise status region */

    if ( h_type < V4 )
    {
        write_char('\n');
    }

}                               /* restart_interp */

/*
 * z_catch
 *
 * Return the value of the frame pointer (FP) for later use with throw.
 * Before V5 games this was a simple pop.
 *
 */

void z_catch( void )
{
    if ( h_type > V4 )
    {
        store_operand( fp );
    }
    else
    {
        sp++;
    }
}                               /* z_catch */

/*
 * z_throw
 *
 * Remove one or more stack frames and return. Works like longjmp, see z_catch.
 *
 */

void z_throw( zword_t value, zword_t new_fp )
{

    if ( new_fp > fp )
    {
        fatal();
    }

    fp = new_fp;

    z_ret( value );

}                               /* z_throw */
