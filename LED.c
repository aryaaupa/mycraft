#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"
#include "LED.h"
#define LED_MASK (1u<<27)
void LED_Init(void){
 // GPIOB is powered by LaunchPad_Init. Do not reset it.
 IOMUX->SECCFG.PINCM[PB27INDEX]=0x81;
 GPIOB->DOUTCLR31_0=LED_MASK;GPIOB->DOE31_0|=LED_MASK;
}
void LED_On(uint32_t data){if(data&1)GPIOB->DOUTSET31_0=LED_MASK;}
void LED_Off(uint32_t data){if(data&1)GPIOB->DOUTCLR31_0=LED_MASK;}
void LED_Toggle(uint32_t data){if(data&1)GPIOB->DOUTTGL31_0=LED_MASK;}
