/* ****************************************************************************
   * Name: Basic delay routines                                               *
   * Author: saper_2                                                          *
   * Contact:                                                                 *
   * Date: 2016-10-29                                                         *
   * Version: 1.1                                                             *
   *                                                                          *
   * License:                                                                 *
   *   MIT                                                                    *
   *   I'm not including license file because it waste of my time & space.    *
   *   Google it.                                                             *
   *                                                                          *
   * About:                                                                   *
   *   Simple basic delays, how precise I never tested :)                     *
   *   This is one of very first units that I created when I was learning     *
   *   C (AVR-GCC), it has been modified with time to make it more precise.   *
   *                                                                          *
   * Bugs:                                                                    *
   * - not tracked.                                                           *
   *                                                                          *
   * Changelog:                                                               *
   * - not tracked.                                                           *
   *                                                                          *
   *                                                                          *
   **************************************************************************** */

#include <avr/io.h>
#include <inttypes.h>
#include "delay.h"

#if (!defined(F_CPU))
	#error F_CPU not defined in compiler parameters! (-D option)
#endif

void delay1us(uint16_t t) {
	while (t>0) {
		#if FCPU25MHZ_ON == 1
		for (uint8_t f25mhz=20;f25mhz>0;f25mhz--) { 
			asm volatile("nop"::);
		}
		#elif FCPU20MHZ_ON == 1
		for (uint8_t f20mhz=20;f20mhz>0;f20mhz--) { 
			asm volatile("nop"::);
		}
		#else
			// ~250ns (271)
			#if F_CPU >= 4000000
				asm volatile("nop"::);
				asm volatile("nop"::);
				asm volatile("nop"::);
			#endif
			
			#if F_CPU >= 6000000
				asm volatile("nop"::);
				asm volatile("nop"::);
			#endif
			
			#if F_CPU > 8000000
				asm volatile("nop"::);
				asm volatile("nop"::);
			#endif
			
			#if F_CPU > 10000000
				asm volatile("nop"::);
				asm volatile("nop"::);
			#endif
			
			#if F_CPU > 12000000
				asm volatile("nop"::);
				asm volatile("nop"::);
			#endif
			
			#if F_CPU > 14000000
				asm volatile("nop"::);
				asm volatile("nop"::);
			#endif
			
			#if F_CPU > 16000000
				asm volatile("nop"::);
				asm volatile("nop"::);
			#endif
			
			// ~250ns (271)
			//asm volatile("nop"::);
			//asm volatile("nop"::);
			//asm volatile("nop"::);
			//asm volatile("nop"::);

			// ~250ns (271)
			//asm volatile("nop"::);
			//asm volatile("nop"::);
			//asm volatile("nop"::);
			//asm volatile("nop"::);

			// ~250ns (271)
			//asm volatile("nop"::);
			//asm volatile("nop"::);
			//asm volatile("nop"::);
			//asm volatile("nop"::);

		#endif
		--t;
	}
}

// /*
void delay1ms(uint16_t t) {
	while (t>0) {
		delay1us(995);
		--t;
	}
}
// */
