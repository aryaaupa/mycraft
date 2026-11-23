#ifndef MYCRAFT_LED_H
#define MYCRAFT_LED_H
#include <stdint.h>
// bit 0 selects the on-board green LED PB27. No external LED required.
void LED_Init(void);
void LED_On(uint32_t data);
void LED_Off(uint32_t data);
void LED_Toggle(uint32_t data);
#endif
