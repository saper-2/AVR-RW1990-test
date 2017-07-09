/* ****************************************************************************
   * Name: 1-Wire bus access routines                                         *
   * Author: saper_2                                                          *
   * Contact:                                                                 *
   * Date: 2017-07-09                                                         *
   * Version: 0.3                                                             *
   *                                                                          *
   * License:                                                                 *
   *   MIT                                                                    *
   *   I'm not including license file because it waste of my time & space.    *
   *   Google it.                                                             *
   *                                                                          *
   * About:                                                                   *
   *   Routines to access Maxim/Dallas 1-wire bus.                            *
   *   ow_delay function must be adjusted for your crystal oscillator,        *
   *   so u get around 1us delay (comment out/uncomment ow_nop's)             *
   *                                                                          *
   * Bugs:                                                                    *
   * - v0.1 - ow_delay must be adjusted for F_CPU                             *
   * - v0.2 - none.                                                           *
   *                                                                          *
   * Changelog:                                                               *
   * - v0.1 - initial version                                                 *
   * - v0.2 - changes for support AS 7 & GCC 4+, replaced ow_delay with       *
   *          _delay_us(x) from util/delay.h                                  *
   * - v0.3 - Added write ID routine for RW1990 (DS1990 clone)                *
   *                                                                          *
   **************************************************************************** */

#include <avr/io.h>
#include <inttypes.h>

#include "1wire.h"
#include "delay.h"



void ow_write_bit(uint8_t bit) {
	// OW_WRB_DELAY_SHORT_1 + OW_WRB_DELAY_LONG_1 < 120us (t_SLOT)
	#define OW_WRB_DELAY_SHORT_1 8 // short delay
	#define OW_WRB_DELAY_LONG_1 72 // long delay

	//_delay_loop_1(3*3); // 3ck = 1us x 3us
	if (bit) { // if we send 1:
		OW_DDR |= 1 << OW_PN; // port=OUT
		OW_PORT &= ~(1 << OW_PN); // port=0
		ow_delay(OW_WRB_DELAY_SHORT_1); // wait "0" and release bus for sending "1":
		OW_DDR &= ~(1 << OW_PN); // port=IN
		OW_PORT |= 1 << OW_PN;
		ow_delay(OW_WRB_DELAY_LONG_1); // wait "1"
	} else {
		OW_DDR |= 1 << OW_PN; // port=OUT
		OW_PORT &= ~(1 << OW_PN); // port=0
		ow_delay(OW_WRB_DELAY_LONG_1); // hold bus for "0" and release bus:
		OW_PORT |= 1 << OW_PN; 
		OW_DDR &= ~(1 << OW_PN); // port=IN
		ow_delay(OW_WRB_DELAY_SHORT_1); // wait for frame to end.
	}
}

uint8_t ow_read_bit(void) {
	static uint8_t tmp;
	
	#define OW_RDB_DELAY_01 1
	#define OW_RDB_DELAY_02 1
	#define OW_RDB_DELAY_03 80
		OW_PORT &= ~(1 << OW_PN); // port=0
		OW_DDR |= 1 << OW_PN; // port=OUT
	ow_delay(OW_RDB_DELAY_01);
		OW_DDR &= ~(1 << OW_PN); //port=IN
	ow_delay(OW_RDB_DELAY_02); // wait for SLAVE to take control over bus
		tmp = (OW_PIN & (1<<OW_PN))>0 ? 1 : 0; // read bus state
	ow_delay(OW_RDB_DELAY_03); // t_release
	
	while (!(OW_PIN & (1<<OW_PN))) {}  // wait for bus release
	
	return tmp;
}

// --------------------------------------
// --------------------------------------
// --------    main functions
// --------------------------------------
// --------------------------------------

uint8_t ow_CalcCRC8(uint8_t inData, uint8_t seed) {
   uint8_t bitsLeft;
   uint8_t temp;

   for (bitsLeft = 8; bitsLeft > 0; bitsLeft--) {
		temp = ((seed ^ inData) & 0x01);
      if (temp == 0) {
         seed >>= 1;
      } else {
         seed ^= 0x18;
         seed >>= 1;
         seed |= 0x80;
      }
      inData >>= 1;
   }
   return seed;    
}

void ow_init(void) {
	OW_DDR &= ~(1 << OW_PN);
	OW_PORT |= 1 << OW_PN;
}

#define OW_RST_DELAY_0 500 // t_RSTL
#define OW_RST_DELAY_1 100 // tR + t_PDH + delay for taking sample
#define OW_RST_DELAY_3 500 // t_RSTH (1/2)
#define OW_RST_DELAY_4 500 // t_RSTH (1/2)

uint8_t ow_reset(void) {
	uint8_t tmp;

	
	OW_DDR |= 1 << OW_PN;
	OW_PORT &= ~(1 << OW_PN);
	// 1W - RESET PULSE
	ow_delay(OW_RST_DELAY_0); // delay 500us
	//_delay_loop_2(2*500);
	// switch port to IN and see what going on
	OW_DDR &= ~(1 << OW_PN); // port=IN
	OW_PORT |= 1 << OW_PN; // port=1 (pull-up)
	ow_delay(OW_RST_DELAY_1); // wait for slave to take control over bus
	// ref: ds2401.pdf Fig.5 -> t_r+t_pdh>60
	//_delay_loop_2(2*30);
	tmp = OW_PIN & (1 << OW_PN) ? 1 : 0; // read bus, if we read 0 then there is something on bus.
	// wait more...
	ow_delay(OW_RST_DELAY_3);
	ow_delay(OW_RST_DELAY_4);

	while (!(OW_PIN & (1<<OW_PN))) {} // wait for bus release...

	return tmp;
}

void ow_write_byte(uint8_t data) {
	uint8_t tmp;
	for (tmp=0;tmp<8;tmp++) {
		ow_write_bit(data & 0x01);
		data >>= 1;
		ow_delay(2);
	}
}

uint8_t ow_read_byte(void) {
	uint8_t tmp, result=0;
	for (tmp=0;tmp<8;tmp++) {
		result >>= 1;
		if (ow_read_bit()) result |= 0x80;
		ow_delay(2);
	}
	return result;
}

#if OW_FUNC_ENABLE_SEARCH_ROM==1
uint8_t ow_rom_search( uint8_t diff, uint8_t *id ) {
	uint8_t i, j, next_diff;
	uint8_t b;

	// error, no device found
	if( ow_reset() ) return 0;
	ow_write_byte(0xF0); // ROM search command
	next_diff = 0;
	i = 8 * 8; // 8 bytes
	do {
		j = 8; // 8 bits
		do {
			b = ow_read_bit(); // read bit
			if (ow_read_bit() && 0x01) { // read complement bit
				if(b && 0x01) return 0;	// 11 - data error
			} else {
				if(!(b && 0x01)) { // 00 = 2 devices
					if( diff > i || ((*id & 0x01) && diff != i) ) {
						b = 1; // now 1
						next_diff = i; // next pass 0
					}
				}
			}
			ow_write_bit(b);  // write bit
			*id >>= 1;
			if (b && 0x01) *id |= 0x80;	 // store bit
			i--;
		} while(--j);
		id++; // next byte
	} while(i);
	return next_diff; // to continue search
}
#endif


#if OW_FUNC_ENABLE_RW1990_WRITE_ROM==1
uint8_t ow_rw1990_write_rom(uint8_t *new) {
	uint8_t res=0x00; // OK
	uint8_t *ptr;
	uint8_t i,j, t;
	uint8_t ds[8];
	ptr = new;

	// write proper family code if mismatch
	#if !defined(OW_RW1990_FAMILY_PRECHECK_DISABLE)
		//ptr=new;
		if (*(ptr) != 0x01) {
			*(ptr) = 0x01; // DS1990 family code
			res |= 0x02;
		}
	#endif

	// check & fix CRC
	#if !defined(OW_RW1990_CRC_PRECHECK_DISABLE)
		uint8_t crc;
		crc=0;
		// verify CRC
		//ptr=new;
		for (i=0;i<7;i++) {
			crc = ow_CalcCRC8((uint8_t)*(ptr), crc);
			ptr++;
		}
		if (crc != ((uint8_t)*(ptr))) {
			(*ptr) = crc;
			// crc fixed
			res |= 0x01;
		}
		// restore ptr
		ptr=new;
	#endif

	if (ow_reset() == 0) {
		// sending skip rom works fine too, but I wonder if this step is even necessary... 
		// we reset bus anyway later... 
		// send: skip rom
		ow_write_byte(0x55);
		// wait
		delay1ms(20);
		if (ow_reset() == 0) {
			ow_write_byte(0xD5);
			// after "WRTIE ROM" command there is a special procedure to write new ROM:
			// each bit of new ID is inverted:
			//   1: pull-down for 60us, release
			//   0: pull-down for 1-5us, release, wait 60us
			// and after each bit the bus must be for at least for 10ms high.
			for (i=0;i<8;i++) {
				t=*(ptr);
				for (j=0;j<8;j++) {
					if (t&0x01) {
						OW_DDR |= 1 << OW_PN; // port=OUT
						OW_PORT &= ~(1 << OW_PN); // port=0
						ow_delay(60); // wait "0" and release bus for sending "1":
						OW_DDR &= ~(1 << OW_PN); // port=IN
						OW_PORT |= 1 << OW_PN;
						ow_delay(3); // wait "0" and release bus for sending "1":
					} else {
						OW_DDR |= 1 << OW_PN; // port=OUT
						OW_PORT &= ~(1 << OW_PN); // port=0
						ow_delay(3); // wait "0" and release bus for sending "1":
						OW_DDR &= ~(1 << OW_PN); // port=IN
						OW_PORT |= 1 << OW_PN;
						ow_delay(60); // wait "0" and release bus for sending "1":
					}
					t >>= 1;
					delay1ms(10); // 10ms write bit 
				} // end for(j)
				ptr++;
			} // end for(i)
			// wait more
			delay1ms(100);
			// verify written rom
			if (ow_reset() == 0) {
				ow_write_byte(0x33); // READ ROM
				// read 8 bytes
				for(i=0;i<8;i++) ds[i]=ow_read_byte();
				// verify if match
				ptr=new;
				t=1;
				for(i=0;i<8;i++) {
					if (*(ptr++)!=ds[i]) t=0;
				}
				if (t==0) {
					res |= 0x80|0x10;
				}
			} else { // ow_reset (post write ID)
				// ow_reset for verify new ID - FAILED
				res |= 0x80|0x08;
			} // ow_reset (post write ID)
		} else { // ow_reset (before write ID)
			// ow_reset before write ID = FAILED
			res |= 0x80|0x04;
		} // ow_reset (before write ID)
	} else { // ow_reset (skip rom)
		// ow_reset for skip rom - FAILED
		res |= 0x80;
	} // ow_reset (skip rom)


	return res;
}
#endif



