//
// Created by Litong Deng on 2022/10/8.
//

#include "tube.h"

#include <8051.h>

#include "common.h"

#define TUBE_SIZE 6
#define DOT 10

unsigned long DIGIT = 0;
unsigned char TUBE_IDX = 0;
unsigned int INTERRUPT_COUNT = 0;
unsigned char INTERRUPT_FLAG = 0;
unsigned int INTERRUPT_MILLIS = 0;

unsigned char DIGIT_SEG_BUFF[] = {0b111111,  0b110,     0b1011011, 0b1001111,
                                  0b1100110, 0b1101101, 0b1111101, 0b111,
                                  0b1111111, 0b1101111};

unsigned char LED_BUFF[] = {
    0xff ^ 0b111111,  0xff ^ 0b110,     0xff ^ 0b1011011, 0xff ^ 0b1001111,
    0xff ^ 0b1100110, 0xff ^ 0b1101101, 0xff ^ 0b1111101, 0xff ^ 0b111,
    0xff ^ 0b1111111, 0xff ^ 0b1101111,
};

void show_increasing_digit_on_tube_with_func_pointer() {
  // 连通 74hc138 引脚使能 3-8 译码器输出以连通各个数码管
  enable_u3_74hc138();
  while (1) {
    run_in_every_ms(100, &show_digit_on_all_tubes);
    DIGIT++;
    if (DIGIT >= 999999) {
      DIGIT = 0;
    }
  }
}

void show_increasing_digit_tube_with_interrupt(unsigned int interrupt_millis) {
  enable_u3_74hc138();

  EA = 1;  // enable global interrupt
  ET0 = 1; // enable Timer0 interrupt

  // setup T0_M1 = 0, T0_M0 = 1 (Timer0 mode TH0-TL0 16 bits timer)
  TMOD = 0x01;
  // setup TH0 TL0 initial value
  TH0 = 0xFC;
  TL0 = 0x67;
  TR0 = 1; // start/enable Timer0

  INTERRUPT_MILLIS = interrupt_millis;
  while (1) {
    if (INTERRUPT_FLAG) {
      INTERRUPT_FLAG = 0;
      DIGIT++;
      if (DIGIT >= 999999) {
        DIGIT = 0;
      }
    }
  }
}

// i: 0 - (TUBE_SIZE-1)
void enable_tube(unsigned char i) {
  // P1_2 P1_1 P1_0
  // TUBE 0 000
  // TUBE 1 001
  // TUBE 2 010
  // TUBE 3 011
  // TUBE 4 100
  // TUBE 5 101
  P1 &= 1 << 3;
  P1 |= i;
}

void turn_on_all_segs() { P0 = 0x00; }

void turn_off_all_segs() { P0 = 0xff; }

unsigned char seg(unsigned char i) { return 0x01 << i; }

/**
 * P0 = 0xff ^ digit_seg(i) shows number i in digital tube
 *
 *        0
 *   ............
 *   .          .
 * 5 .          .   1
 *   .    6     .
 *   ............
 *   .          .
 * 4 .          .   2
 *   .          .
 *   ............
 *        3
 *                  .   7
 */
unsigned char digit_seg(unsigned char i) {
  switch (i) {
  case 0:
    return seg(0) | seg(5) | seg(1) | seg(4) | seg(3) | seg(2);
  case 1:
    return seg(1) | seg(2);
  case 2:
    return seg(0) | seg(1) | seg(6) | seg(4) | seg(3);
  case 3:
    return seg(0) | seg(1) | seg(2) | seg(3) | seg(6);
  case 4:
    return seg(5) | seg(6) | seg(1) | seg(2);
  case 5:
    return seg(0) | seg(5) | seg(6) | seg(2) | seg(3);
  case 6:
    return seg(0) | seg(5) | seg(6) | seg(2) | seg(3) | seg(4);
  case 7:
    return seg(0) | seg(1) | seg(2);
  case 8:
    return seg(0) | seg(5) | seg(6) | seg(1) | seg(4) | seg(3) | seg(2);
  case 9:
    return seg(0) | seg(5) | seg(6) | seg(1) | seg(3) | seg(2);
  case DOT:
    // show .
    return seg(7);
  default:
    return 0x00;
  }
}

void show_digit(unsigned char i) {
  // P0 = 0xff ^ digit_seg(i);
  // use array buffer to accelerate since the value is not changed in run-time.
  P0 = LED_BUFF[i];
}

void show_digit_on_all_tubes() {
  turn_off_all_segs();
  switch (TUBE_IDX) {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
    enable_tube(TUBE_IDX);
    show_digit((DIGIT / pow(10, TUBE_IDX)) % 10);
    TUBE_IDX++;
    break;
  case 5:
    enable_tube(TUBE_IDX);
    show_digit((DIGIT / pow(10, TUBE_IDX)) % 10);
    TUBE_IDX = 0;
    break;
  default:
    break;
  }

  // 下面这个 for 循环在单片机上的 0 位数码管不闪烁，用上面的 switch 循环可以,
  // 具体原因未知...

  // for (unsigned char TUBE_IDX = 0; TUBE_IDX < TUBE_SIZE; TUBE_IDX++) {
  //   // 依次连通各个数码管，一次只能点亮一个数码管
  //   enable_tube(TUBE_IDX);
  //   // 在数码管上展示数字 DIGIT 运算需要时间，
  //   // 引入中断机制，来避免算术运算时影响间隔时间而产生在LED上的交叉影响
  //   // 单片机执行的时候也需要考虑到语句的耗时周期
  //   show_digit((DIGIT / pow(10, TUBE_IDX)) % 10);
  // }
}

unsigned int pow(unsigned int x, unsigned int y) {
  unsigned int res = 1;
  while (y--) {
    res *= x;
  }
  return res;
}

// 中断函数 函数原型需要加上 __interrupt(x)，x 对应不同中断
// 中断函数不需要调用，达到中断时自动进入
// 定时器 T0 产生溢出时触发一个 T0 中断，对应函数标号为 1
void InterruptTime0_tube() __interrupt(1) {
  // setup TH0 TL0 initial value, each interrupt(Timer0 overflow) will pass
  // 1ms
  TH0 = 0xFC;
  TL0 = 0x67;

  INTERRUPT_COUNT++;
  if (INTERRUPT_COUNT >= INTERRUPT_MILLIS) { // 1ms * INTERRUPT_MILLIS
    INTERRUPT_COUNT = 0;
    INTERRUPT_FLAG = 1;
  }
  show_digit_on_all_tubes();
}
