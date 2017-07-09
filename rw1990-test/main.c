/* ****************************************************************************
   * Name: RW1990 test                                                        *
   * Author: saper_2                                                          *
   * Contact:                                                                 *
   * Date: 2017-07-09                                                         *
   * Version: 0.1                                                             *
   *                                                                          *
   * License:                                                                 *
   *   MIT                                                                    *
   *   I'm not including license file because it waste of my time & space.    *
   *   Google it.                                                             *
   *                                                                          *
   * About:                                                                   *
   *   Test for RW1990 - a clone of DS1990A with ability to reprogram         *
   *   it's unique ID. In original Maxim (Dallas) DS1990A the unique ID is    *
   *   laser-etched in die at manufacturing process.                          *
   *                                                                          *
   *                                                                          *
   * Bugs:                                                                    *
   * - v0.0  - none.                                                          *
   *                                                                          *
   * Changelog:                                                               *
   * - v0.1  - initial release.                                               *
   *                                                                          *
   *                                                                          *
   **************************************************************************** */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <avr/interrupt.h>

#include "lib/delay.h"
//#include "lib/lcd-graphic.h"
#include "lib/usart.h"
#include "lib/1wire.h"

#define led_on PORTB |= 1<<0
#define led_off PORTB &= ~(1<<0)
#define led_toggle PORTB ^= 1<<0

#define led2_on PORTB |= 1<<1
#define led2_off PORTB &= ~(1<<1)

// -----------------------------------------------------------------
// -----------------------------------------------------------------
//     Commands

#define CMD_NONE 0x00
#define CMD_1W_SCAN 0x01
#define CMD_1W_READ_ROM 0x02
#define CMD_1W_WRITE_RW1990 0xF0

volatile uint8_t command = CMD_NONE;

#define COMMAND_SET(xcmd) command=xcmd

// new rom buffer
uint8_t rw1990_new_rom[8];

// -----------------------------------------------------------------
// -----------------------------------------------------------------
//                   USART Rx
// -----------------------------------------------------------------
// -----------------------------------------------------------------
#define RS_BUFF_SIZE 24
uint8_t rsBuffPtr = 0;
uint8_t rsBuff[RS_BUFF_SIZE];

void flushRSBuff(void) {
	for (uint8_t i=0;i<RS_BUFF_SIZE;i++) rsBuff[i]=0;
	rsBuffPtr=0;
	#ifdef DEBUG_MODE
	usart_send_strP(PSTR("<<<RS-FLUSH>>>"));
	#endif
}

uint8_t hex2dec(char hex) {
	if (hex > 0x2f && hex < 0x3a) return hex-0x30;
	if (hex > 0x40 && hex < 0x47) return hex-0x37;
	if (hex > 0x60 && hex < 0x67) return hex-0x57;
	return 0x0f;
}

uint8_t bcd2bin(uint8_t bcd) {
	uint8_t res=(bcd&0x0f);
	bcd >>= 4;
	res += (bcd&0x0f)*10;
	return res;
}

void usart_crlf(void) {
	usart_send_char(0x0d);
	usart_send_char(0x0a);
}

void processRSBuff(void) {
	uint8_t tmp=0, crc8;
	
	// scan bus
	if (rsBuff[0] == 's') {
		//tmp = hex2dec(rsBuff[1]) & 0x0f;
		//tmp = (tmp<<4) | (hex2dec(rsBuff[2]) & 0x0f);
		COMMAND_SET(CMD_1W_SCAN);
		//usart_send_char('q');
		//usart_send_hex_byte(tmp);
		//usart_crlf();
	} else if (rsBuff[0] == 'r') {
		// read rom
		COMMAND_SET(CMD_1W_READ_ROM);
		//usart_send_char('r');
		//usart_crlf();
	} else if (rsBuff[0] == 'n') {
		// set new rom, this command is send back to PC
		usart_send_char('n');
		crc8=0;
		for(uint8_t i=0;i<8;i++) {
			tmp = hex2dec(rsBuff[(i*2)+1]); // +1 for cmd char offset
			tmp = (tmp<<4) | (hex2dec(rsBuff[(i*2)+1+1]) & 0x0f); // +1 for cmd char offset, +1 for 2nd hex byte nibble
			rw1990_new_rom[i] = tmp;
			if (i<7) crc8=ow_CalcCRC8(tmp,crc8);
			usart_send_hex_byte(tmp);
		}
		usart_send_char(':');
		// verify CRC
		if (crc8==rw1990_new_rom[7]) {
			usart_send_char('1');
		} else {
			// do not match, clear rw1990_new_rom
			//for(uint8_t i=0;i<8;i++) rw1990_new_rom[i]=0;
			usart_send_char('0');
			usart_send_char(':');
			usart_send_hex_byte(crc8);
		}
		usart_crlf();
	} else if (rsBuff[0] == 'm') {
		// query for new rom
		usart_send_char('m');
		crc8=0;
		for(uint8_t i=0;i<8;i++) {
			tmp = rw1990_new_rom[i];
			if (i<7) crc8=ow_CalcCRC8(tmp,crc8);
			usart_send_hex_byte(tmp);
		}
		usart_send_char(':');
		// verify CRC
		if (crc8==rw1990_new_rom[7]) {
			usart_send_char('1');
		} else {
			usart_send_char('0');
			usart_send_char(':');
			usart_send_hex_byte(crc8);
		}
		usart_crlf();
	} else if (rsBuff[0] == 'w') {
		// write new rom, only if rw1990_new_rom don't have zeros
		usart_send_char('w');
		tmp=0;
		for(uint8_t i=0;i<8;i++) {
			if (rw1990_new_rom[i] == 0x00) tmp++;
		}
		// tmp must be less than 6
		if (tmp < 6) {
			usart_send_char('1'); // OK set command to be executed
			COMMAND_SET(CMD_1W_WRITE_RW1990);
		} else {
			usart_send_char('0'); // niope....
		}
		usart_crlf();
	} else if (rsBuff[0] == '?') {
		usart_send_strP(PSTR("CMD:s=scan:r=readROM:nXX8=setNewROM:m=PrintNewROM:w=writeNewROM\r\n"));
		usart_send_strP(PSTR("VER:1WLIB=03:SW=01\r\n"));
	}
	flushRSBuff();
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
//                         USART Rx End.
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// scan 1w bus
void bus_scan(void) {
	uint8_t t, ds[8], crc8;
	
	usart_send_strP(PSTR("1W:SCAN\r\n"));
	if (ow_reset() == 0) {
		usart_send_strP(PSTR("1W:RESET=1\r\n"));
		// reset successful, read IDs
		t=0xff;
		do {
			// get ID
			t=ow_rom_search(t,&ds[0]);
			// send data to terminal
			usart_send_strP(PSTR("1W:SCAN:"));
			usart_send_hex_byte(t);
			usart_send_char(':');
			crc8=0;
			for(uint8_t i=0;i<8;i++) {
				if (i< 7) crc8 = ow_CalcCRC8(ds[i], crc8);
				usart_send_hex_byte(ds[i]);
			}
			if (crc8 == ds[7]) {
				usart_send_strP(PSTR(":OK"));
			} else {
				usart_send_strP(PSTR(":ER:"));
				usart_send_hex_byte(crc8);
			}
			usart_crlf();
		
		} while (t!=0x00);
	
	} else {
		usart_send_strP(PSTR("1W:RESET=0\r\n"));
	}
}

// read first device rom...
void read_rom(void) {
	uint8_t ds[8], crc8;
	
	usart_send_strP(PSTR("1W:READ-ROM\r\n"));
	if (ow_reset() == 0) {
		usart_send_strP(PSTR("1W:RESET=1\r\n"));
		// reset successful
		ow_write_byte(0x33); // READ ROM COMMAND
		// get 8 bytes
		for (uint8_t i=0;i<8;i++) {
			ds[i] = ow_read_byte();
		}
		// print out
		usart_send_strP(PSTR("1W:READ-ROM:"));
		crc8=0;
		for (uint8_t i=0;i<8;i++) {
			if (i<7) crc8 = ow_CalcCRC8(ds[i], crc8);
			usart_send_hex_byte(ds[i]);
		}

		if (crc8 == ds[7]) {
			usart_send_strP(PSTR(":OK"));
		} else {
			usart_send_strP(PSTR(":ER:"));
			usart_send_hex_byte(crc8);
		}
		usart_crlf();

	} else {
		usart_send_strP(PSTR("1W:RESET=0\r\n"));
	}
}

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
// can return : 0x00 = ok, 0x01=OK but crc fixed, 0x80=error
/*uint8_t rw1990_write_new_rom(uint8_t *new) {
	uint8_t res=0x00; // OK
	uint8_t *ptr;
	uint8_t i,j, t;
	ptr = new;

	// write proper family code if mismatch
	#if !defined(OW_RW1990_FAMILY_PRECHECK_DISABLE)
		//ptr=new;
		if (*(ptr) != 0x01) {
			*(ptr) = 0x01; // DS1990 family code
			res |= 0x02;
		}
	#endif

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
		// send skip rom
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
				}
				ptr++;
			}
			// wait more
			delay1ms(100);
			// verify written rom
			if (ow_reset() == 0) {
				uint8_t ds[8];
				ow_write_byte(0x33); // READ ROM
				// read 8 bytes
				for(i=0;i<8;i++) ds[i]=ow_read_byte();
				// verify if match
				ptr=new;
				t=1;
				usart_send_strP(PSTR("1W:RW1990-WR:VRF:"));
				for(i=0;i<8;i++) {
					if (*(ptr++)!=ds[i]) t=0;
					usart_send_hex_byte(ds[i]);
				}
				usart_send_char(':');
				if (t==0) {
					res |= 0x80|0x10;
					usart_send_char('0');
				} else {
					usart_send_char('1');
				}
				usart_crlf();
			} else {
				res |= 0x80|0x08;
			}
		} else {
			res |= 0x80|0x04;
		}
		// reset 
	} else {
		res |= 0x80;
	}


	return res;
}*/

int main(void)
{
	uint8_t t;//, ds[8], x, y;
	DDRB = 1<<0 | 1<<1;
	led_off;
	led2_off;
	
	for(uint8_t i=0;i<8;i++) rw1990_new_rom[i]=0;

	ow_init();

	sei();
	usart_config(25,USART_RX_ENABLE|USART_TX_ENABLE|USART_RX_INT_COMPLET,USART_MODE_8N1);
	usart_send_strP(PSTR("\r\n\r\nRW1990:START\r\n"));

	while (1) 
    {
		led_on;
		delay1ms(50);
		
		if (rsBuffPtr == 254) {
			processRSBuff();
		}

		if (command != CMD_NONE) {
			// called command bus scan
			if (command == CMD_1W_SCAN) {
				bus_scan();
			} else if (command == CMD_1W_READ_ROM) {
				read_rom();
			} else if (command == CMD_1W_WRITE_RW1990) {
				// perform 1w write
				// check if rw1990_new_rom is not 0x00
				t=0;
				for(uint8_t i=0;i<8;i++) {
					if (rw1990_new_rom[i] == 0x00) t++;
				}
				if (t < 6) {
					// print new rom
					usart_send_strP(PSTR("1W:RW1990-WR:NEW:"));
					for (uint8_t i=0;i<8;i++) usart_send_hex_byte(rw1990_new_rom[i]);
					usart_crlf();
					// write rom
					led2_on;
					t = ow_rw1990_write_rom(&rw1990_new_rom[0]);
					
					usart_send_strP(PSTR("1W:RW1990-WR:RES="));
					usart_send_hex_byte(t);
					usart_crlf();
					delay1ms(50);
					led2_off;
				} else {
					usart_send_strP(PSTR("1W:RW1990-WR:ZEROS\r\n"));	
				}
			}
			// reset command
			COMMAND_SET(CMD_NONE);
		}


		if (ow_reset() == 0) {
			delay1ms(100);
		}
		led_off;
		delay1ms(350);
    }
}


SIGNAL(USART_RXC_vect) {
	char c = UDR;
	
	if (c == 0x0d) { // CR
		rsBuffPtr=254; // mark as buffer to be processed
		} else if (c == 0x1b) { // ESC
		flushRSBuff();
		} else {
		if (rsBuffPtr<RS_BUFF_SIZE) {
			rsBuff[rsBuffPtr]=c;
			rsBuffPtr++;
		}
	}
}
