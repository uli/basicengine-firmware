
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

void AZIP::open_story( void )
{
    int16_t count;
    uint32_t pos = 0;
    char memory_name[] = "MEMORY.DAT";
    char game_name[] = "GAME.DAT";
    Unifile game_in;

    if (!(game = Unifile::open( memory_name, FILE_OVERWRITE )))
    {
        fatal(PSTR("cannot open mem\n"));
    }

    if ( game_in = Unifile::open( game_name, FILE_READ ) )
    {
        game_in.seekSet(0);
        while ( ( count = game_in.read( (char *)stack, sizeof(stack)) ) > 0 )
        {
            game.write( (char *)stack, count);
            pos += count;
        }

        game_in.close();
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
    int status = 0;

    /* Get the file name */
    status = 1;

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
    int status;

    status = 1;

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
    store_operand( ( zword_t ) - 1 );

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
    store_operand( ( zword_t ) - 1 );

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

struct cache_block * AZIP::fetch_block(unsigned long addr)
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

zbyte_t AZIP::get_byte(unsigned long offset){ unsigned long addr = offset; return read_data_byte(&addr); }
zword_t AZIP::get_word(unsigned long offset){ unsigned long addr = offset; return read_data_word(&addr);}
void AZIP::set_byte(unsigned long offset, zbyte_t value){ unsigned long addr = offset; write_data_byte(&addr, value);}
void AZIP::set_word(unsigned long offset, zword_t value){ unsigned long addr = offset; write_data_word(&addr, value);}

