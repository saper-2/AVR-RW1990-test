/* ****************************************************************************
   *                                                                          *
   * Name    : Header files for some basic delays                             *
   * Author  : saper_2                                                        *
   * Contact :                                                                *
   * Date    : 2011-07-11                                                     *
   * Version : 0.1                                                            *
   * More    : Check  *.c file.                                               *
   *                                                                          *
   * Notice  : In case of receiving F_CPU not defined error, you must define  *
   *           F_CPU=xxxxxxxxUL in compiler symbols (-D option)               *
   *                                                                          *
   *                                                                          *
   **************************************************************************** */
#ifndef DELAY_H_INCLUDED
#define DELAY_H_INCLUDED

// set to 1 if F_CPU is 25MHz else must be 0
#define FCPU25MHZ_ON 0
// set to 1 if F_CPU is 20MHz else must be 0
#define FCPU20MHZ_ON 0

#define NOP() {asm volatile("nop"::);}

#define nop asm volatile("nop"::);

void delay1ms(uint16_t t);
void delay1us(uint16_t t);

#endif // delay_h_included

