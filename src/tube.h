//
// Created by Litong Deng on 2022/10/8.
//  数码管
//

#ifndef MCU_STC89_TUBE_H
#define MCU_STC89_TUBE_H

#include <8051.h>

void show_increasing_digit_on_tube_with_func_pointer();

void show_increasing_digit_tube_with_interrupt(unsigned int interrupt_millis);

void enable_tube(unsigned char i);

void turn_on_all_segs();

void turn_off_all_segs();

unsigned char seg(unsigned char i);

unsigned char digit_seg(unsigned char i);

void show_digit(unsigned char i);

void show_digit_on_all_tubes();

unsigned int pow(unsigned int x, unsigned int y);

void InterruptTime0_tube() __interrupt(1);

#endif // MCU_STC89_TUBE_H
