# rw1990-test
This project came to live because I found on aliexpress cheap clones of DS1990 that allow to change their ID  :grinning:

## IDE
Atmel Studio 7 with AVR-GCC toolchain 5.4.0

## Hardware
* Vcc: 5V
* MCU: ATMega32
* F_CPU: 16MHz
* IO:
	* PD7: 1-Wire bus with 4,7k pullup
	* PB0: LED 1 (H=led on , L=led off) - blink short when nothing is on 1W bus, long - when someting is on bus or commands are exec.
	* PB1: LED 2 (H=led off, L=led off) - is on when RW1990 is programmed
* USART: 38400bps 8N1

## Software
The RW1990-test read is controlled via USART commands. 
At CPU reset/Power on MCU sends to host PC via USART "hello" :smile:
```\r\n\r\nRW1990:START\r\n```
(```\r``` - is CR character (carriage return) , ```\n``` - is LF character (line feed))
Those commands are:
* ```?``` - send command list with few character description and 1wire lib version
* ```s``` - scan 1-Wire bus for devices
* ```r``` - read 1st device on bus ROM ID
* ```n``` - set new RW1990 ROM ID
* ```m``` - print new RW1990 ROM ID
* ```w``` - perform real write to RW1990 of new ROM ID

### Commands explanation
Each command is confirmed with only CR (ASCII: 0x0D = 13dec = Carriage return)

Not every one command will respond with itself. Whenever 1-W bus is accessed (1-Wire reset) there will be returned line with it's result in format:
```1W:RESET=x```
Where **x** is 
* **0** = no 1-Wire slave(s) response to reset
* **1** = 1-Wire slave(s) responded to reset

Command ```?``` will not be explained - it's just too simple :smile:

#### Command ```s``` - scan 1-Wire bus
No parameters after command, will return lines:
```1W:SCAN\r\n``` and followed by ```1W:RESET=x\r\n``` . If rest is OK ("1") then all IDs of slave devices connected to bus will be printed in format (each device ID in separate line):
```1W:SCAN:[scan_bit_mask_1byte]:[device_id_8bytes]:<OK/ER:CALC_CRC>\r\n```
Where:
* **scan_bit_mask_1byte** - a 1 byte in HEX with bit mask for getting next device ID , if=0x00 then no more devices on bus
* **device_id_8bytes** - 8 bytes in HEX with device ID (family code, 6 bytes od unique ID (48bits) and CRC)
* **<OK/ER:CALC_CRC>** - if CRC is valid will be "OK", if not then will be "ER:" and followed by a byte in HEX with valid CRC

#### Command ```r``` - read ROM
No parameters after command, will return lines:
```1W:READ-ROM\r\n``` and followed by ```1W:RESET=x\r\n``` . If rest is OK ("1") then 1st device on bus ID will be returned:
```1W:READ-ROM:[device_id_8bytes]:<OK/ER:CALC_CRC>\r\n```

Fields meaning linke in **scan command**.

#### Command ```n``` - set new ROM ID
Set new rom ID for RW1990.
Command letter is followed by 8 bytes in HEX format with new RW1990 ID:
```nXXXXXXXXXXXXXXXX\r```
Where **XXXXXXXXXXXXXXXX** is the new ID in HEX format
ATmega will replay with command, newd ID and CRC info:
```nXXXXXXXXXXXXXXXX:<1/0:valid_crc>\r```
Where:
* **XXXXXXXXXXXXXXXX** is the new ID in HEX format
* **<1/0:valid_crc>** - if "1" then CRC is valid, otherwise return "0" followed by HEX byte with valid CRC for given ID (for 7 first bytes)

#### Command ```m``` - print new ROM ID
Send to host new ROM ID.
No parameters after command, will return lines:
```mXXXXXXXXXXXXXXXX:<1/0:valid_crc>\r\n```
Where:
* **XXXXXXXXXXXXXXXX** is the new ID in HEX format
* **<1/0:valid_crc>** - if "1" then CRC is valid, otherwise return "0" followed by HEX byte with valid CRC for given ID (for 7 first bytes)

#### Command ```w``` - Write new ROM ID to RW1990
Start write of new ROM ID to RW1990.
No parameters after command, will return lines:
```w<1/0>\r\n```
Where:
* **<1/0>** - if "1" then ROM ID is OK (does not contains only "00"), 0 - new ROM ID is full of 0 and write won't be executed.

##### Write
Write is performed in few steps:
1. First is checkd if there is no "0" only in new ROM ID.
	* If there are only zeros will be returned line: ```1W:RW1990-WR:ZEROS\r\n``` and write will not be performed
1. Next is printed line with ROM ID to be written: ```1W:RW1990-WR:NEW:XXXXXXXXXXXXXXXX\r\n```
1. After this is called function ```ow_rw1990_write_rom(uint8_t *new)``` with parameter containing array of byte[8] with new ROM ID, In function:
	1. There is enabled checking & correcting for Family code & CRC. If CRC and/or FamilyCode does not match then they are corrected before write - this is reported by 2 LSB bits in result.
	2. Now, is performed 1st 1-Wire reset, followed by **SKIP ROM** command,
	3. After **SKIP ROM** is short delay and then is performed 2nd 1-Wire reset,
	4. Now is issued command **WRITE ROM** 0xD5, and after command are being sent 8 bytes (64bits) of new ROM ID in special manner *(see code for more info)*
	5. After sending new ID there is a short delay,
	6. Now is performed 3rd 1-Wire reset,
	7. After bus reset is issued command **READ ROM**
	8. And the new ROM ID is being read from RW1990
	9. the read ROM ID is then compared with ROM ID that was written, if those match then no error bits are set.

The result of write function from 1-Wire lib is returned in next line (after *1W:RW1990-WR:NEW*):
```1W:RW1990-WR:RES=XX```
Where XX is a byte in HEX format, which bits represent, not used bits are 0:
```
0b0xxxxxxx = OK
0bxxxxxxx1 = CRC was fixed
0bxxxxxx1x = Family code was fixed
0b1xxxxxxx = No presence pulse from 1st 1W-reset
0b1xxxx1xx = No presence pulse from 2nd 1W-reset after skip ROM
0b1xxx1xxx = No presence pulse from 3rd 1W-reset after writing new ROM
0b1xx1xxxx = Verify write FAILED (new read rom mismach)
```
So the result 0x00 , 0x01 , 0x02 , 0x03 - mean that RW1990 was programmed OK, but for 0x01..0x03:
* 0x01 - OK but CRC was corrected
* 0x02 - OK but Family code was corrected to 01 (DS1990A family code)
* 0x03 - OK but Family code & CRC was corrected.

If bit 7th is set (mask 0x80 ) then there occured errors, mask byte with bits above to see what error occured.

# EoF
In forlder ```images``` you can see my test board :D
The file rw1990-test_m32_16MHz_1w-PD7_USART38400_8N1.hex is a compiled flash for ATMega32 with hardware configured as above mentioned.
:sleeping:





