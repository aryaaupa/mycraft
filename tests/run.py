from pathlib import Path
import subprocess,tempfile,os
root=Path(__file__).resolve().parents[1]
s=root.joinpath('Lab9Main.c').read_text()
s='\n'.join(l for l in s.splitlines() if not l.startswith('#include'))
s=s.replace('int main(void)','int firmware_main(void)')
stub=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#define INITR_REDTAB 0
static struct {struct {uint32_t IIDX;} CPU_INT;} timer;
#define TIMG12 (&timer)
static uint32_t mockButtons,mockAdc=2048;
static uint32_t __get_PRIMASK(void){return 0;}
static void __disable_irq(void){}
static void __enable_irq(void){}
static void __set_PRIMASK(uint32_t x){(void)x;}
static uint32_t ADCin(void){return mockAdc;}
static uint32_t Switch_In(void){return mockButtons;}
static void ADCinit(void){}
static void Switch_Init(void){}
static void LaunchPad_Init(void){}
static void Clock_Init_HFXT_40_80MHz(int x){(void)x;}
static void TimerG12_IntArm(uint32_t a,uint32_t b){(void)a;(void)b;}
static void LED_Init(void){}
static void LED_On(uint32_t x){(void)x;}
static void LED_Off(uint32_t x){(void)x;}
static void Clock_Delay1ms(uint32_t x){(void)x;}
static void Sound_Init(void){}
static void Sound_Shoot(void){}
static void Sound_Highpitch(void){}
static void Sound_Killed(void){}
static void Sound_Explosion(void){}
static void ST7735_InitR(int x){(void)x;}
static uint16_t fb[160][128],textColor;
static int curx,cury;
static char textlog[10000];static unsigned textlen;
static void ST7735_DrawPixel(int x,int y,uint16_t c){if(x>=0&&x<128&&y>=0&&y<160)fb[y][x]=c;}
static void ST7735_FillRect(int x,int y,int w,int h,uint16_t c){for(int j=0;j<h;j++)for(int i=0;i<w;i++)ST7735_DrawPixel(x+i,y+j,c);}
static void ST7735_DrawFastHLine(int x,int y,int w,uint16_t c){ST7735_FillRect(x,y,w,1,c);}
static void ST7735_DrawFastVLine(int x,int y,int h,uint16_t c){ST7735_FillRect(x,y,1,h,c);}
static void ST7735_FillScreen(uint16_t c){ST7735_FillRect(0,0,128,160,c);textlen=0;}
static void ST7735_SetTextColor(uint16_t c){textColor=c;}
static void ST7735_SetCursor(int x,int y){curx=x;cury=y;}
static void ST7735_OutString(char *s){if(textlen<9000)textlen+=snprintf(textlog+textlen,sizeof(textlog)-textlen,"%d\t%d\t%u\t%s\n",curx*6,cury*8,textColor,s);}
static void ST7735_OutUDec(uint32_t n){char b[16];snprintf(b,sizeof b,"%u",n);ST7735_OutString(b);}
static void capture(const char *name){char path[100];snprintf(path,sizeof path,"%s.ppm",name);FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n128 160\n255\n");for(int y=0;y<160;y++)for(int x=0;x<128;x++){uint16_t c=fb[y][x];fputc(((c>>11)&31)*255/31,f);fputc(((c>>5)&63)*255/63,f);fputc((c&31)*255/31,f);}fclose(f);snprintf(path,sizeof path,"%s_text.tsv",name);f=fopen(path,"w");fwrite(textlog,1,textlen,f);fclose(f);}
'''
tests=r'''
int main(void){
 Input input;timer.CPU_INT.IIDX=1;
 mockButtons=MINE;TIMG12_IRQHandler();assert(pop(&input)&&input.edges==0);
 TIMG12_IRQHandler();assert(pop(&input)&&input.edges==MINE);
 TIMG12_IRQHandler();assert(pop(&input)&&input.edges==0);
 mockButtons=0;TIMG12_IRQHandler();pop(&input);TIMG12_IRQHandler();assert(pop(&input)&&input.held==0);
 for(int i=0;i<20;i++)TIMG12_IRQHandler();assert(MyCraft_DroppedInputs==5);
 int count=0;while(pop(&input))count++;assert(count==15);
 worldInit();state=1;playerX=4;playerY=10;stock[0]=1;facing=1;tick=1;
 update(0,BUILD,2048);assert(built==1&&stock[0]==0&&placed[10][5]);
 update(0,MINE,2048);assert(built==0&&stock[0]==1&&score==0);
 for(int i=0;i<100;i++){update(0,BUILD,2048);update(0,MINE,2048);}assert(score==0&&built==0);
 worldInit();state=1;playerX=8;playerY=10;tick=1;facing=1;
 update(0,MINE,2048);assert(diamonds==1&&score==40);
 diamonds=3;built=4;stock[0]=1;update(0,BUILD,2048);assert(state==2&&built==5);
 worldInit();assert(score==0&&diamonds==0&&built==0&&hp==3);
 state=1;hp=0;update(0,0,2048);assert(state==3);
 for(int trial=0;trial<20;trial++){
  worldInit();state=1;
  for(int i=0;i<5000&&state==1;i++){
   tick++;uint32_t r=random32();update(r&15,r&15,r%4096);
   assert(playerX>=0&&playerX<W&&playerY>=2&&playerY<H);
   unsigned n=0;for(int y=0;y<H;y++)for(int x=0;x<W;x++)n+=placed[y][x];assert(n==built);
   if(i%8==0)draw();
  }
 }
 lang=0;menu();capture("menu");
 instructions();capture("instructions");
 worldInit();ST7735_FillScreen(0xC73F);draw();capture("game");
 puts("PASS: debounce, queue overflow, recycling, gems, win/loss, reset, randomized bounds and building counts");
}
'''
with tempfile.TemporaryDirectory() as d:
 f=Path(d)/'check.c';f.write_text(stub+s+tests)
 exe=Path(d)/'check'
 subprocess.run(['gcc','-std=c11','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(f),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],cwd=root,check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})
audio=root.joinpath('Sound.c').read_text()
audio='\n'.join(l for l in audio.splitlines() if not l.startswith('#include'))
audio_stub=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static struct {uint32_t CTRL,LOAD,VAL;} systick;
static struct {uint32_t ICSR;} scb;
#define SysTick (&systick)
#define SCB (&scb)
#define SysTick_IRQn 0
#define SCB_ICSR_PENDSTCLR_Msk 0x2000000u
#define SysTick_CTRL_CLKSOURCE_Msk 4u
#define SysTick_CTRL_ENABLE_Msk 1u
#define SysTick_CTRL_TICKINT_Msk 2u
static uint32_t mask,dac;
static uint32_t __get_PRIMASK(void){return mask;}
static void __disable_irq(void){mask=1;}
static void __set_PRIMASK(uint32_t n){mask=n;}
static void NVIC_SetPriority(int a,int b){(void)a;(void)b;}
static void DAC5_Init(void){}
static void DAC5_Out(uint32_t n){assert(n<32);dac=n;}
'''
audio_test=r'''
int main(void){
 const uint8_t samples[]={3,29};Sound_Init();assert(dac==16&&SysTick->CTRL==0);
 Sound_Start(samples,2);assert(mask==0&&remaining==2);
 SysTick_Handler();assert(dac==3&&remaining==1);
 SysTick_Handler();assert(dac==29&&remaining==0&&SysTick->CTRL==7);
 SysTick_Handler();assert(dac==16&&SysTick->CTRL==0);
 mask=1;Sound_Start(samples,2);assert(mask==1);Sound_Start(0,2);assert(remaining==0&&dac==16);
 Sound_Shoot();for(int i=0;i<1000;i++)SysTick_Handler();assert(dac==16);
 Sound_Highpitch();Sound_Killed();assert(remaining==sizeof(Build));
 puts("PASS: PCM bounds, final-sample hold, silence, cancellation, replacement, interrupt-mask restoration");
}
'''
with tempfile.TemporaryDirectory() as d:
 f=Path(d)/'audio.c';f.write_text(audio_stub+root.joinpath('CozySounds.h').read_text()+audio+audio_test)
 exe=Path(d)/'audio'
 subprocess.run(['gcc','-std=c11','-fsanitize=address,undefined',str(f),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
