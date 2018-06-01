
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
  * property.c
  *
  * Property manipulation routines
  *
  */

#include "ztypes.h"

/*
* get_property_addr
*
* Calculate the address of the start of the property list associated with an
* object.
*
*/

static zword_t get_property_addr( zword_t obj )
{
    zword_t object_addr;
    zword_t prop_addr;
    zbyte_t size;

    /* Calculate the address of the property pointer in the object */

    object_addr = get_object_address( obj );
    object_addr += ( h_type <= V3 ) ? O3_PROPERTY_OFFSET : O4_PROPERTY_OFFSET;

    /* Read the property address */
    prop_addr = get_word( object_addr );

    /* Skip past object description which is an ASCIC of encoded words */
    size = get_byte( prop_addr );

    return prop_addr + ( size * 2 ) + 1;

}                               /* get_property_addr */

/*
* get_next_property
*
* Calculate the address of the next property in a property list.
*
*/

static zword_t get_next_property( zword_t prop_addr )
{
    zbyte_t value;

    /* Load the current property id */
    value = get_byte( prop_addr );
    prop_addr++;

    /* Calculate the length of this property */

    if ( h_type <= V3 )
        value >>= 5;
    else if ( !( value & 0x80 ) )
        value >>= 6;
    else
    {
        value = get_byte( prop_addr );
        value &= property_size_mask;

        if ( value == 0 )
            value = 64;            /* spec 1.0 */
    }

    /* Address property length to current property pointer */
    return prop_addr + value + 1;

}                               /* get_next_property */

/*
* z_get_prop
*
* Load a property from a property list. Properties are held in list sorted by
* property id, with highest ids first. There is also a concept of a default
* property for loading only. The default properties are held in a table pointed
* to be the object pointer, and occupy the space before the first object.
*
*/

void z_get_prop( zword_t obj, zword_t prop )
{
    zword_t prop_addr;
    zword_t wprop_val;
    zbyte_t bprop_val;
    zbyte_t value;

    /* Load address of first property */
    prop_addr = get_property_addr( obj );

    /* Scan down the property list */
    for ( ;; )
    {
        value = get_byte( prop_addr );
        if ( ( zbyte_t ) ( value & property_mask ) <= ( zbyte_t ) prop )
            break;
        prop_addr = get_next_property( prop_addr );
    }

    /* If the property ids match then load the first property */

    if ( ( zbyte_t ) ( value & property_mask ) == ( zbyte_t ) prop ) /* property found */
    {
        prop_addr++;
        /* Only load first property if it is a byte sized property */
        if ( ((h_type <= V3) && !( value & 0xe0 )) || ((h_type >= V4) && !( value & 0xc0 )) )
        {
            bprop_val = get_byte( prop_addr );
            wprop_val = bprop_val;
        }
        else
        {
            wprop_val = get_word( prop_addr );
        }
    }
    else                         /* property not found */
    {
        /* Calculate the address of the default property */
        prop_addr = h_objects_offset + ( ( prop - 1 ) * 2 );
        wprop_val = get_word( prop_addr );
    }

    /* store the property value */

    store_operand( wprop_val );

}                               /* z_get_prop */

/*
* z_put_prop
*
* Store a property value in a property list. The property must exist in the
* property list to be replaced.
*
*/

void z_put_prop( zword_t obj, zword_t prop, zword_t setvalue )
{
    zword_t prop_addr;
    zword_t value;

    /* Load address of first property */
    prop_addr = get_property_addr( obj );

    /* Scan down the property list */
    for ( ;; )
    {
        value = get_byte( prop_addr );
        if ( ( value & property_mask ) <= prop )
            break;
        prop_addr = get_next_property( prop_addr );
    }

    /* If the property id was found, store a new value, otherwise complain */

    if ( ( value & property_mask ) != prop )
    {
        fatal();
    }

    /* Determine if this is a byte or word sized property */

    prop_addr++;

    if ( ((h_type <= V3) && !( value & 0xe0 )) || ((h_type >= V4) && !( value & 0xc0 )) )
    {
        set_byte( prop_addr, ( zbyte_t ) setvalue );
    }
    else
    {
        set_word( prop_addr, ( zword_t ) setvalue );
    }

}                               /* z_put_prop */

/*
* z_get_next_prop
*
* Load the property after the current property. If the current property is zero
* then load the first property.
*
*/

void z_get_next_prop( zword_t obj, zword_t prop )
{
    zword_t prop_addr;
    zbyte_t value;

   if(obj == 0)
   {
       store_operand( ( zword_t ) ( 0 ) );
       return;
   }

    /* Load address of first property */
    prop_addr = get_property_addr( obj );

    /* If the property id is non zero then find the next property */
    if ( prop != 0 )
    {
        /* Scan down the property list while the target property id is less
        * than the property id in the list */
        do
        {
            value = get_byte( prop_addr );
            prop_addr = get_next_property( prop_addr );
        }
        while ( ( zbyte_t ) ( value & property_mask ) > ( zbyte_t ) prop );

        /* If the property id wasn't found then complain */
        if ( ( zbyte_t ) ( value & property_mask ) != ( zbyte_t ) prop )
        {
            fatal();
        }
    }

    /* Return the next property id */
    value = get_byte( prop_addr );
    store_operand( ( zword_t ) ( value & property_mask ) );

}                               /* z_get_next_prop */

/*
* z_get_prop_addr
*
* Load the address address of the data associated with a property.
*
*/

void z_get_prop_addr( zword_t obj, zword_t prop )
{
    zword_t prop_addr;
    zbyte_t value;

    /* Load address of first property */
    prop_addr = get_property_addr( obj );

    /* Scan down the property list */
    for ( ;; )
    {
        value = get_byte( prop_addr );
        if ( ( zbyte_t ) ( value & property_mask ) <= ( zbyte_t ) prop )
            break;
        prop_addr = get_next_property( prop_addr );
    }

    /* If the property id was found, calc the prop addr, else return zero */

    if ( ( zbyte_t ) ( value & property_mask ) == ( zbyte_t ) prop )
    {
        /* Skip past property id, can be a byte or a word */

        if ( h_type >= V4 && ( value & 0x80 ) )
        {
            prop_addr++;
        }
        store_operand( ( zword_t ) ( prop_addr + 1 ) );
    }
    else
    {
        /* No property found, just return 0 */
        store_operand( 0 );
    }

}                               /* z_get_prop_addr */

/*
* z_get_prop_len
*
* Load the length of a property.
*
*/

void z_get_prop_len( zword_t prop_addr )
{
    zbyte_t value;

    /* This is proper according to an email to the Zmachine list by Graham*/
    if ( prop_addr == 0 )
    {
        store_operand( ( zword_t ) 0 );
        return;
    }

    /* Back up the property pointer to the property id */
    prop_addr--;
    value = get_byte( prop_addr );

    if ( h_type <= V3 )
    {
        value = ( zbyte_t ) ( ( value >> ( zbyte_t ) 5 ) + ( zbyte_t ) 1 );
    }
    else if ( !( value & 0x80 ) )
    {
        value = ( zbyte_t ) ( ( value >> ( zbyte_t ) 6 ) + ( zbyte_t ) 1 );
    }
    else
    {
        value &= ( zbyte_t ) property_size_mask;

        if ( value == 0 )
            value = ( zbyte_t ) 64; /* spec 1.0 */
    }

    store_operand( value );

}                               /* z_get_prop_len */

/*
* z_scan_table
*
* Scan an array of bytes or words looking for a target byte or word. The
* optional 4th parameter can set the address step and also whether to scan a
* byte array.
*
*/

void z_scan_table( int argc, zword_t * argv )
{
    unsigned long address;
    unsigned int i, step;

    /* Supply default parameters */

    if ( argc < 4 )
        argv[3] = 0x82;

    address = argv[1];
    step = argv[3];

    /* Check size bit (bit 7 of step, 1 = word, 0 = byte) */

    if ( step & 0x80 )
    {

        step &= 0x7f;

        /* Scan down an array for count words looking for a match */

        for ( i = 0; i < argv[2]; i++ )
        {

            /* If the word was found store its address and jump */

            if ( read_data_word( &address ) == argv[0] )
            {
                store_operand( ( zword_t ) ( address - 2 ) );
                conditional_jump( TRUE );
                return;
            }

            /* Back up address then step by increment */

            address = ( address - 2 ) + step;

        }

    }
    else
    {

        step &= 0x7f;

        /* Scan down an array for count bytes looking for a match */

        for ( i = 0; i < argv[2]; i++ )
        {

            /* If the byte was found store its address and jump */

            if ( ( zword_t ) read_data_byte( &address ) == ( zword_t ) argv[0] )
            {
                store_operand( ( zword_t ) ( address - 1 ) );
                conditional_jump( TRUE );
                return;
            }

            /* Back up address then step by increment */

            address = ( address - 1 ) + step;

        }

    }

    /* If the data was not found store zero and jump */

    store_operand( 0 );
    conditional_jump( FALSE );

}                               /* z_scan_table */

/*
* z_copy_table
*
*/

void z_copy_table( zword_t src, zword_t dst, zword_t count )
{
    unsigned long address;
    unsigned int i;

    /* Catch no-op move case */

    if ( src == dst || count == 0 )
        return;

    /* If destination address is zero then fill source with zeros */

    if ( dst == 0 )
    {
        for ( i = 0; i < count; i++ )
            z_storeb( src++, 0, 0 );
        return;
    }

    address = src;

    if ( ( ZINT16 ) count < 0 )
    {
        while ( count++ )
            z_storeb( dst++, 0, read_data_byte( &address ) );
    }
    else
    {
        address += ( unsigned long ) count;
        dst += count;
        while ( count-- )
        {
            address--;
            z_storeb( --dst, 0, read_data_byte( &address ) );
            address--;
        }
    }

}                               /* z_copy_table */

/*
* z_loadw
*
* Load a word from an array of words
*
*/

void z_loadw( zword_t addr, zword_t offset )
{
    unsigned long address;
   ZINT16 _offset = (ZINT16)offset;

    /* Calculate word array index address */

   address = addr + ( _offset * 2 );

    /* Store the byte */

    store_operand( read_data_word( &address ) );

}                               /* z_loadw */

/*
* z_loadb
*
* Load a byte from an array of bytes
*
*/

void z_loadb( zword_t addr, zword_t offset )
{
    unsigned long address;
   ZINT16 _offset = (ZINT16)offset;

    /* Calculate byte array index address */

   address = addr + _offset;

    /* Load the byte */

    store_operand( read_data_byte( &address ) );

}                               /* z_loadb */

/*
* z_storew
*
* Store a word in an array of words
*
*/

void z_storew( zword_t addr, zword_t offset, zword_t value )
{

    /* Calculate word array index address */

    addr += offset * 2;

    /* Store the word */

    set_word( addr, value );

}                               /* z_storew */

/*
* z_storeb
*
* Store a byte in an array of bytes
*
*/

void z_storeb( zword_t addr, zword_t offset, zword_t value )
{

    /* Calculate byte array index address */

    addr += offset;

    /* Store the byte */

    set_byte( addr, (zbyte_t)value );

}                               /* z_storeb */
