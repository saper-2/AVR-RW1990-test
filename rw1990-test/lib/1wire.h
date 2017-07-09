/* ****************************************************************************
   *                                                                          *
   * Name    : Header file for 1-wire bus access                              *
   * Author  : saper_2                                                        *
   * Contact :                                                                *
   * Date    : 2017-07-09                                                     *
   * Version : 0.3                                                            *
   * More    : Check  *.c file.                                               *
   *                                                                          *
   **************************************************************************** */
#ifndef _1_WIRE_H_
#define _1_WIRE_H_

#define OW_PORT PORTD
#define OW_PIN PIND
#define OW_DDR DDRD
#define OW_PN 7

// overwrite ow_delay to util/delay.h:_delay_us(xxx)
#include <util/delay.h>
#define ow_delay(delay) _delay_us(delay)


#define ow_nop() { asm volatile("nop"::); }


uint8_t ow_CalcCRC8(uint8_t inData, uint8_t seed);
void ow_init(void);
uint8_t ow_reset(void);
void ow_write_byte(uint8_t data);
uint8_t ow_read_byte(void);

// set to 1 to enable function to search for 1wire slaves on bus
#define OW_FUNC_ENABLE_SEARCH_ROM 1

#if OW_FUNC_ENABLE_SEARCH_ROM==1
	uint8_t ow_rom_search( uint8_t diff, uint8_t *id );
#endif

// set to 1 to enable support for writing ID to RW1990 (DS1990A clone)
#define OW_FUNC_ENABLE_RW1990_WRITE_ROM 1

#if OW_FUNC_ENABLE_RW1990_WRITE_ROM==1
//#define OW_RW1990_CRC_PRECHECK_DISABLE
//#define OW_RW1990_FAMILY_PRECHECK_DISABLE
/*
	Function   : rw1990_write_new_rom
	Description: Write new ROM to RW1990
	Parameters : *new - pointer to array of byte[8] with new ROM value
	Returns    : In case of error the MSB bit will be set, other bits will 
				 be set according to place of error occurrence
				 0b0xxxxxxx = OK
				 0bxxxxxxx1 = CRC was fixed
				 0bxxxxxx1x = Family code was fixed
				 0b1xxxxxxx = No presence pulse from 1W-reset
				 0b1xxxx1XX = No presence pulse from 1W-reset after skip ROM
				 0b1xxx1xXX = No presence pulse from 1W-reset after writing new ROM
				 0b1xx1xxXX = Verify write FAILED (new read rom mismach)
*/
uint8_t ow_rw1990_write_rom(uint8_t *new);
#endif

#endif

