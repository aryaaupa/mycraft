#include <stdint.h>
#include <ti/devices/msp/msp.h>
void DAC5_Init(void){
 const uint8_t pins[5]={11,12,14,15,16};
 for(unsigned i=0;i<5;i++)IOMUX->SECCFG.PINCM[pins[i]]=0x81;
 GPIOB->DOUTCLR31_0=31; GPIOB->DOE31_0|=31;
}
void DAC5_Out(uint32_t data){
 // Single writer owns PB0..4; only DAC bits can toggle.
 GPIOB->DOUTTGL31_0=(GPIOB->DOUT31_0^(data&31u))&31u;
}
