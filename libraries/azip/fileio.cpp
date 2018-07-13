
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
 * fileio.c
 *
 * File manipulation routines. Should be generic.
 *
 */

#include "ztypes.h"

/* Static data */

/*
 * open_story
 *
 * Open game file for read.
 *
 */

void AZIP::open_story( const char *game_name )
{
    int16_t count;
    uint32_t pos = 0;
    BString memory_name(F("MEMORY.DAT"));
    Unifile game_in;

    if (!(game = Unifile::open( memory_name.c_str(), UFILE_OVERWRITE )))
    {
        fatal(PSTR("cannot open mem\n"));
    }

    if ( game_in = Unifile::open( game_name, UFILE_READ ) )
    {
        game_in.seekSet(0);
        while ( ( count = game_in.read( (char *)stack, sizeof(stack)) ) > 0 )
        {
            game.write( (char *)stack, count);
            pos += count;
        }

        game_in.close();
        game.sync();    // avoids file system errors if azip isn't properly exited
        return;
    }
    fatal(PSTR("nopen game"));
}                               /* open_story */


/*
 * close_story
 *
 * Close game file if open.
 *
 */

void AZIP::flush_block(int i) {
    struct cache_block *cb = &cache_blocks[i];
    if (cb->dirty) {
        game.seekSet(cb->addr);
        game.write((char *)cb->mem, 512);
        cb->dirty = false;
    }
}

void AZIP::close_story( void )
{

    if ( game )
    {
        for (int i = 0; i < CACHE_BLOCKS; ++i)
            flush_block(i);
        game.close( );
    }

}                               /* close_story */

/*
 * get_story_size
 *
 * Calculate the size of the game file. Only used for very old games that do not
 * have the game file size in the header.
 *
 */

unsigned int AZIP::get_story_size( void )
{

    return game.fileSize( );

}                               /* get_story_size */


/*
 * z_verify
 *
 * Verify game ($verify verb). Add all bytes in game file except for bytes in
 * the game file header.
 *
 */

void AZIP::z_verify( void )
{
    /* Make a conditional jump based on whether the checksum is equal */

    conditional_jump( TRUE );

}                               /* z_verify */

#define SAVE_NAME     "story.sav" /* Default save name */
#define SCRIPT_NAME   "story.scr" /* Default script name */
#define RECORD_NAME   "story.rec" /* Default record name */
#define AUXILARY_NAME "story.aux" /* Default auxilary name */

/*
 * get_file_name
 *
 * Return the name of a file. Flag can be one of:
 *    GAME_SAVE     - Save file (write only)
 *    GAME_RESTORE  - Save file (read only)
 *    GAME_SCRIPT   - Script file (write only)
 *    GAME_RECORD   - Keystroke record file (write only)
 *    GAME_PLABACK  - Keystroke record file (read only)
 *    GAME_SAVE_AUX - Auxilary (preferred settings) file (write only)
 *    GAME_LOAD_AUX - Auxilary (preferred settings) file (read only)
 */

int AZIP::get_file_name( char *file_name, char *default_name, int flag )
{
    int status = 0;

    /* If no default file name then supply the standard name */

    if ( default_name[0] == '\0' )
    {
        if ( flag == GAME_SCRIPT )
            strcpy_P( default_name, PSTR(SCRIPT_NAME) );
        else if ( flag == GAME_RECORD || flag == GAME_PLAYBACK )
            strcpy_P( default_name, PSTR(RECORD_NAME) );
        else if ( flag == GAME_SAVE_AUX || GAME_LOAD_AUX )
            strcpy_P( default_name, PSTR(AUXILARY_NAME) );
        else                    /* (flag == GAME_SAVE || flag == GAME_RESTORE) */
            strcpy_P( default_name, PSTR(SAVE_NAME) );
    }

    /* Prompt for the file name */

    write_string( PSTR("Enter a file name.\n") );
    write_string( PSTR("(Default is \"") );
    write_string( default_name );
    write_string( PSTR("\"): ") );

    zword_t buffer = h_file_size;
    set_byte( buffer, 127);
    set_byte( buffer + 1, 0);
    ( void ) get_line( buffer, 0, 0 );

    /* Copy file name from the input buffer */

    if ( h_type > V4 )
    {
        unsigned char len = get_byte( buffer + 1 );

        for (int i = 0; i < len; ++i)
            file_name[i] = get_byte( buffer + 2 + i);
        file_name[len] = '\0';
    }
    else {
        char c; int i;
        for (i = 0; (c = get_byte( buffer + 1 + i)); ++i)
            file_name[i] = c;
        file_name[i] = 0;
    }

    /* If nothing typed then use the default name */

    if ( file_name[0] == '\0' )
        strcpy( file_name, default_name );

    /* Check if we are going to overwrite the file */

    if ( flag == GAME_SAVE || flag == GAME_SCRIPT || flag == GAME_RECORD || flag == GAME_SAVE_AUX )
    {
        if ( Unifile::exists( file_name ) ) {
            /* If it succeeded then prompt to overwrite */

            write_string( PSTR("You are about to write over an existing file.\n") );
            write_string( PSTR("Proceed? (Y/N) ") );

            char c;
            do
            {
                c = ( char ) input_character( 0 );
                c = ( char ) toupper( c );
            }
            while ( c != 'Y' && c != 'N' );

            write_char( c );
            newline(  );

            /* If no overwrite then fail the routine */

            if ( c == 'N' )
                status = 1;
        }
    }

    return ( status );

}                               /* get_file_name */

/*
 * save_restore
 *
 * Common save and restore code. Just save or restore the game stack and the
 * writeable data area.
 *
 */

int AZIP::save_restore( const char *file_name, int flag )
{
    Unifile tfp;

    int scripting_flag = 0, status = 0;

    /* Open the save file and disable scripting */

    if ( flag == GAME_SAVE || flag == GAME_RESTORE )
    {
        if ( !( tfp = Unifile::open( file_name, ( flag == GAME_SAVE ) ? UFILE_OVERWRITE : UFILE_READ ) ) )
        {
            write_string( PSTR("Cannot open SAVE file ") ); write_string(file_name); newline();
            return ( 1 );
        }
        scripting_flag = get_word( H_FLAGS ) & SCRIPTING_FLAG;
        set_word( H_FLAGS, get_word( H_FLAGS ) & ( ~SCRIPTING_FLAG ) );
    }

    /* Push PC, FP, version and store SP in special location */

    stack[--sp] = ( zword_t ) ( pc / PAGE_SIZE );
    stack[--sp] = ( zword_t ) ( pc % PAGE_SIZE );
    stack[--sp] = fp;
    stack[--sp] = h_version;
    stack[0] = sp;

    /* Save or restore stack */

    if ( flag == GAME_SAVE )
    {
        if ( status == 0 && tfp.write( (char *)stack, sizeof ( stack ) ) != sizeof ( stack ) )
            status = 1;
    }
    else if ( flag == GAME_RESTORE )
    {
        if ( status == 0 && tfp.read( (char *)stack, sizeof ( stack ) ) != sizeof ( stack ) )
            status = 1;
    }

    /* Restore SP, check version, restore FP and PC */

    sp = stack[0];

    if ( stack[sp++] != h_version )
    {
        fatal( PSTR("save_restore(): Wrong game or version") );
    }

    fp = stack[sp++];
    pc = stack[sp++];
    pc += ( unsigned long ) stack[sp++] * PAGE_SIZE;

    /* Save or restore writeable game data area */

    if ( flag == GAME_SAVE )
    {
        if ( status == 0 ) {
            for (int i = 0; i < h_restart_size; ++i) {
                if ( tfp.write( get_byte( i ) ) < 0 ) {
                    status = 1;
                    break;
                }
            }
        }
    }
    else if ( flag == GAME_RESTORE )
    {
        if ( status == 0 ) {
            for (int i = 0; i < h_restart_size; ++i) {
                int c = tfp.read();
                if (c < 0) {
                    status = 1;
                    break;
                } else {
                    set_byte( i, c );
                }
            }
        }
    }

    /* Close the save file and restore scripting */

    if ( flag == GAME_SAVE )
    {
        tfp.close();
        if ( scripting_flag )
        {
            set_word( H_FLAGS, get_word( H_FLAGS ) | SCRIPTING_FLAG );
        }
    }
    else if ( flag == GAME_RESTORE )
    {
        tfp.close();
        restart_screen(  );
        restart_interp( scripting_flag );
    }

    /* Handle read or write errors */

    if ( status )
    {
        if ( flag == GAME_SAVE )
        {
            write_string( PSTR("Write to SAVE file failed\n") );
            Unifile::remove( file_name );
        }
        else
        {
            fatal( PSTR("save_restore(): Read from SAVE file failed") );
        }
    }

    return ( status );

}                               /* save_restore */

/*
 * get_default_name
 *
 * Read a default file name from the memory of the Z-machine and
 * copy it to an array.
 *
 */

void AZIP::get_default_name( char *default_name, zword_t addr )
{
    zbyte_t len;
    zbyte_t c;
    unsigned int i;

    if ( addr != 0 )
    {
        len = get_byte( addr );

        for ( i = 0; i < len; i++ )
        {
            addr++;
            c = get_byte( addr );
            default_name[i] = c;
        }
        default_name[i] = 0;

        if ( strchr( default_name, '.' ) == 0 )
        {
            strcpy( default_name + i, ".aux" );
        }

    }
    else
    {
        strcpy( default_name, auxilary_name );
    }

}                               /* get_default_name */

/*
 * z_save
 *
 * Saves data to disk. Returns:
 *     0 = save failed
 *     1 = save succeeded
 *
 */

int AZIP::z_save( int argc, zword_t table, zword_t bytes, zword_t name )
{
    char new_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];
    char default_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];
    Unifile afp;

    int status = 0;

    if ( argc == 3 )
    {
        get_default_name( default_name, name );
        if ( get_file_name( new_name, default_name, GAME_SAVE_AUX ) != 0 )
        {
            goto finished;
        }

        if ( !( afp = Unifile::open( new_name, UFILE_OVERWRITE ) ) )
        {
            goto finished;
        }

        for (int i = 0; i < bytes; ++i) {
            if ( afp.write( get_byte( i + table ) ) < 0) {
                status = 1;
                break;
            }
        }

        afp.close();

        if ( status == 0 )
        {
            strcpy( auxilary_name, default_name );
        }
    }
    else
    {
        /* Get the file name */
        status = 1;

        if ( get_file_name( new_name, save_name, GAME_SAVE ) == 0 )
        {
            /* Do a save operation */
            if ( save_restore( new_name, GAME_SAVE ) == 0 )
            {
                /* Save the new name as the default file name */
                strcpy( save_name, new_name );

                /* Indicate success */
                status = 0;
            }
        }
    }

finished:

    /* Return result of save to Z-code */

    if ( h_type < V4 )
    {
        conditional_jump( status == 0 );
    }
    else
    {
        store_operand( (zword_t)(( status == 0 ) ? 1 : 0) );
    }

    return ( status );
}                               /* z_save */


/*
 * z_restore
 *
 * Restore game state from disk. Returns:
 *     0 = restore failed
 *     2 = restore succeeded
 */

int AZIP::z_restore( int argc, zword_t table, zword_t bytes, zword_t name )
{
    char new_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];
    char default_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];
    Unifile afp;

    int status;

    status = 0;

    if ( argc == 3 )
    {
        get_default_name( default_name, name );
        if ( get_file_name( new_name, default_name, GAME_LOAD_AUX ) == 0 )
        {
            goto finished;
        }

        if ( !( afp = Unifile::open( new_name, UFILE_READ ) ) )
        {
            goto finished;
        }

        for (int i = 0; i < bytes; ++i) {
            int c = afp.read();
            if ( c < 0 ) {
                status = 1;
                break;
            } else {
                set_byte( table + i, c );
            }
        }

        afp.close();

        if ( status == 0 )
        {
            strcpy( auxilary_name, default_name );
        }
    }
    else
    {
        /* Get the file name */
        status = 1;
        if ( get_file_name( new_name, save_name, GAME_RESTORE ) == 0 )
        {
            /* Do the restore operation */
            if ( save_restore( new_name, GAME_RESTORE ) == 0 )
            {
                /* Reset the status region (this is just for Seastalker) */
#if 0
                if ( h_type < V4 )
                {
                    z_split_window( 0 );
                    blank_status_line(  );
                }
#endif

                /* Save the new name as the default file name */
                strcpy( save_name, new_name );

                /* Indicate success */
                status = 0;
            }
        }
    }

finished:
    /* Return result of save to Z-code */

    if ( h_type < V4 )
    {
        conditional_jump( status == 0 );
    }
    else
    {
        store_operand( (zword_t)(( status == 0 ) ? 2 : 0) );
    }

    return ( status );
}                               /* z_restore */

/*
 * z_save_undo
 *
 * Save the current Z machine state in memory for a future undo. Returns:
 *    -1 = feature unavailable
 *     0 = save failed
 *     1 = save succeeded
 *
 */

void AZIP::z_save_undo( void )
{
    /* If no memory for data area then say undo is not available */
    store_operand( ( zword_t ) -1 );

}                               /* z_save_undo */

/*
 * z_restore_undo
 *
 * Restore the current Z machine state from memory. Returns:
 *    -1 = feature unavailable
 *     0 = restore failed
 *     2 = restore succeeded
 *
 */

void AZIP::z_restore_undo( void )
{
    /* If no memory for data area then say undo is not available */
    store_operand( ( zword_t ) -1 );

}                               /* z_restore_undo */


/*
 * read_code_word
 *
 * Read a word from the instruction stream.
 *
 */

zword_t AZIP::read_code_word( void )
{
    zword_t w;

    w = ( zword_t ) read_code_byte(  ) << 8;
    w |= ( zword_t ) read_code_byte(  );

    return ( w );

}                               /* read_code_word */

struct cache_block *AZIP::fetch_block(unsigned long addr)
{
    int bl = rand() % CACHE_BLOCKS;
    flush_block(bl);
    struct cache_block *cb = &cache_blocks[bl];
    cb->addr = addr / 512 * 512;
    game.seekSet( cb->addr );
    game.read((char *)cb->mem, 512);
    return cb;
}

/*
 * read_code_byte
 *
 * Read a byte from the instruction stream.
 *
 */

zbyte_t AZIP::read_code_byte( void )
{
    zbyte_t value;

    for (int i = 0; i < CACHE_BLOCKS; ++i) {
        struct cache_block *cb = &cache_blocks[i];
        if (cb->addr / 512 == pc / 512) {
            value = cb->mem[pc % 512];
            pc++;
            return value;
        }
    }

    struct cache_block *cb = fetch_block(pc);
    value = cb->mem[pc % 512];

    /* Update the PC */
    pc++;

    /* Return byte from page offset */
    return value;

}                               /* read_code_byte */

/*
 * read_data_word
 *
 * Read a word from the data area.
 *
 */

zword_t AZIP::read_data_word( unsigned long *addr )
{
    zword_t w;

    w = ( zword_t ) read_data_byte( addr ) << 8;
    w |= ( zword_t ) read_data_byte( addr );

    return ( w );

}                               /* read_data_word */

void AZIP::write_data_word( unsigned long *addr, zword_t value)
{
    write_data_byte(addr, (zbyte_t)(value >> 8));
    write_data_byte(addr, (zbyte_t)(value));

}                               /* write_data_word */

/*
 * read_data_byte
 *
 * Read a byte from the data area.
 *
 */

zbyte_t AZIP::read_data_byte( unsigned long *addr )
{
    zbyte_t value = 0;

    for (int i = 0; i < CACHE_BLOCKS; ++i) {
        struct cache_block *cb = &cache_blocks[i];
        if (cb->addr / 512 == *addr / 512) {
            value = cb->mem[*addr % 512];
            (*addr)++;
            return value;
        }
    }
    struct cache_block *cb = fetch_block(*addr);
    value = cb->mem[*addr % 512];

    /* Update the address */

    ( *addr )++;

    return ( value );

}                               /* read_data_byte */

void AZIP::write_data_byte( unsigned long *addr, zbyte_t value)
{
    for (int i = 0; i < CACHE_BLOCKS; ++i) {
        struct cache_block *cb = &cache_blocks[i];
        if (cb->addr / 512 == *addr / 512) {
            cb->mem[*addr % 512] = value;
            cb->dirty = true;
            (*addr)++;
            return;
        }
    }
    struct cache_block *cb = fetch_block(*addr);
    cb->mem[*addr % 512] = value;
    cb->dirty = true;

    /* Update the address */
    ( *addr )++;

}                               /* write_data_byte */

zbyte_t AZIP::get_byte(unsigned long offset){
    unsigned long addr = offset; return read_data_byte(&addr);
}
zword_t AZIP::get_word(unsigned long offset){
    unsigned long addr = offset; return read_data_word(&addr);
}
void AZIP::set_byte(unsigned long offset, zbyte_t value){
    unsigned long addr = offset; write_data_byte(&addr, value);
}
void AZIP::set_word(unsigned long offset, zword_t value){
    unsigned long addr = offset; write_data_word(&addr, value);
}

