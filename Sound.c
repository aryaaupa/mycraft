// Replace existing Sound.c. Sound_Start now accepts 5-bit PCM (0..31).
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/DAC5.h"
#include "Sound.h"
#include "CozySounds.h"
static const uint8_t *volatile cursor;
static volatile uint32_t remaining;
void Sound_Init(void){
 DAC5_Init(); DAC5_Out(16);
 cursor=0;remaining=0;
 SysTick->CTRL=0;SysTick->LOAD=(80000000u+5500u)/11000u-1u;
 SysTick->VAL=0;NVIC_SetPriority(SysTick_IRQn,1);
 SCB->ICSR=SCB_ICSR_PENDSTCLR_Msk;
}
void Sound_Start(const uint8_t *pt,uint32_t count){
 uint32_t key=__get_PRIMASK();__disable_irq();
 SysTick->CTRL=0;SCB->ICSR=SCB_ICSR_PENDSTCLR_Msk;
 cursor=pt;remaining=pt?count:0;SysTick->VAL=0;
 if(remaining)SysTick->CTRL=SysTick_CTRL_CLKSOURCE_Msk|SysTick_CTRL_ENABLE_Msk|SysTick_CTRL_TICKINT_Msk;
 else DAC5_Out(16);
 __set_PRIMASK(key);
}
void SysTick_Handler(void){
 if(remaining){DAC5_Out(*cursor++);remaining--;}
 else {SysTick->CTRL=0;cursor=0;DAC5_Out(16);}
 // The final sample is held for a full sample period before returning to midscale.
}
#define PLAY(a) Sound_Start(a,(uint32_t)sizeof(a))
void Sound_Shoot(void){PLAY(Mine);}
void Sound_Killed(void){PLAY(Build);}
void Sound_Explosion(void){PLAY(Hurt);}
void Sound_Highpitch(void){PLAY(Win);}
void Sound_Fastinvader1(void){PLAY(Build);}
void Sound_Fastinvader2(void){PLAY(Mine);}
void Sound_Fastinvader3(void){PLAY(Build);}
void Sound_Fastinvader4(void){PLAY(Win);}
