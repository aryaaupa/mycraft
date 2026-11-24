// MYCRAFT: A Little Home for Muchi. Course-concept implementation, not official course code.
// See COURSE_MAP.md and CHECKOUT_GUIDE.md. No heap, floating point, framework or framebuffer.
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/Timer.h"
#include "../inc/ADC1.h"
#include "Switch.h"
#include "Sound.h"
#include "LED.h"
#define UP 1u
#define DOWN 2u
#define MINE 4u
#define BUILD 8u
#define CREAM 0xFF9C
#define INK 0x294A
#define SKY 0xC73F
#define PINK 0xFBB7
#define RED 0xE986
#define YELLOW 0xFF26
#define GRASS 0x8DF1
#define CPU_HZ 80000000u
#ifndef MYCRAFT_LCD_TAB
#define MYCRAFT_LCD_TAB INITR_REDTAB
#endif
#ifndef MYCRAFT_TEST_MODE
#define MYCRAFT_TEST_MODE 5
#endif
static const char nickname[]="MUCHI";
typedef enum {MENU,HELP,PLAY,ENDING} Screen;
static Screen screen=MENU;
static uint8_t lang,helpPage;
static const char *word(const char *en,const char *es){if(lang)return es;return en;}
// A single-slot mailbox and semaphore, following the course foreground/background pattern.
// Only this ISR acquires inputs. Main copies/clears them in a very short critical section.
static volatile uint32_t adcSample,buttons,presses,ready;
volatile uint32_t MyCraft_MissedFrames;
static uint8_t debounce[4];
void TIMG12_IRQHandler(void){
 if(TIMG12->CPU_INT.IIDX==1){
  uint32_t raw=Switch_In();adcSample=ADCin();
  for(uint32_t i=0;i<4;i++){
   uint32_t bit=1u<<i;
   if((raw&bit)==(buttons&bit))debounce[i]=0;
   else{debounce[i]++;if(debounce[i]==2){buttons^=bit;debounce[i]=0;if(buttons&bit)presses|=bit;}}
  }
  if(ready)MyCraft_MissedFrames++;
  ready=1;
 }
}
// Fixed 5x7 pixel font; explicit pixel positioning, independent of driver cursor spacing.
static const uint8_t font5x7[][7]={
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14}};
static void label(int x,int y,const char *text,uint16_t color,int scale){
 for(;*text;text++,x+=6*scale){
  unsigned char c=(unsigned char)*text;
  if(c>='a'&&c<='z')c-=32;
  int index=c>='A'&&c<='Z'?c-'A':c>='0'&&c<='9'?26+c-'0':-1;
  for(int row=0;row<7;row++){
   uint8_t bits=index>=0?font5x7[index][row]:0;
   if(c==':')bits=(row==2||row==5)?4:0;
   if(c=='/')bits=(row==0?1:row==1?2:row<4?4:row==4?8:16);
   if(c=='-')bits=row==3?14:0;
   if(c=='+')bits=row==3?14:(row==2||row==4)?4:0;
   if(c=='>')bits=row==1?8:row==2?4:row==3?2:row==4?4:row==5?8:0;
   if(c=='!')bits=row<5||row==6?4:0;
   if(c=='=')bits=row==2||row==4?14:0;
   if(c=='.')bits=row==6?4:0;
   if(c==39)bits=row<2?4:0;
   for(int col=0;col<5;col++)if(bits&(16>>col)){
    int px=x+col*scale,py=y+row*scale;
    if(px>=0&&py>=0&&px+scale<=128&&py+scale<=160)
     ST7735_FillRect(px,py,scale,scale,color);
   }
  }
 }
}
static void str(int x,int y,const char *s,uint16_t c){label(x*6,y*8,s,c,1);}
static void digits(int x,int y,uint32_t n,int scale){
 char text[6]; uint32_t place=10000; int i=0,started=0;
 while(place){uint32_t digit=n/place;n=n%place;
  if(digit||started||place==1){text[i++]='0'+digit;started=1;}place=place/10;
 }text[i]=0;label(x,y,text,0x294A,scale);
}
static void number(int x,int y,uint32_t n){digits(x*6,y*8,n,1);}
static const char minerArt[8][9]={
 "01111110",
 "01222210",
 "01262610",
 "01222210",
 "03333330",
 "43333334",
 "04400440",
 "05500550"
};
static const char slimeArt[8][9]={
 "00000000",
 "00033000",
 "00333300",
 "03333330",
 "33333333",
 "33633633",
 "33333333",
 "04444440"
};
static const char rabbitArt[8][9]={
 "02002000",
 "02002000",
 "02222000",
 "02262000",
 "22222200",
 "22622220",
 "02222200",
 "02002000"
};
static const char butterflyArt[8][9]={
 "00000000",
 "33000330",
 "33303330",
 "03333300",
 "00344300",
 "03333300",
 "33000330",
 "00000000"
};
static uint16_t spriteColor(char n,uint8_t kind){
 if(kind==1){ // green slime
   switch(n){case '3':return 0xB6B4;case '6':return 0x0000;case '4':return 0x1B42;default:return 0xFFFF;}
 }
 if(kind==2){ // white rabbit
   switch(n){case '2':return 0xFFFF;case '6':return 0xF9B6;default:return 0xFFFF;}
 }
 if(kind==3){ // butterfly
   switch(n){case '3':return 0xF99F;case '4':return 0x4208;default:return 0xFFFF;}
 }
 switch(n){case '1':return 0x39C7;case '2':return 0xFCAE;
 case '3':return 0xFBB7;case '4':return 0xFCAE;
 case '5':return 0x21A9;case '6':return 0x0000;default:return 0xFFFF;}
}
static void sprite8(int sx,int sy,const char art[8][9],uint8_t kind){
 int x,y;
 for(y=0;y<8;y++)for(x=0;x<8;x++){
   char n=art[y][x];
   if(n!='0')ST7735_DrawPixel(sx+x,sy+y,spriteColor(n,kind));
 }
}

static void heart(int x,int y,uint16_t c){
 ST7735_FillRect(x,y,2,2,c);ST7735_FillRect(x+3,y,2,2,c);
 ST7735_FillRect(x,y+2,5,2,c);ST7735_FillRect(x+1,y+4,3,1,c);ST7735_DrawPixel(x+2,y+5,c);
}
// Fixed coordinates make every objective reachable and allow a short demo.
static const int16_t woodX[5]={14,35,58,82,108};
static const int16_t flowerX[3]={24,61,101};
static uint8_t collectedWood[5],collectedFlower[3];
static uint8_t wood,house,flowers,phase,invincible,hint;
static uint16_t score,bumps;
static uint32_t tick;
static int playerX=4,playerY=101,slimeX=90,rabbitX=12,butterflyX=55;
static int slimeDirection=-1,rabbitDirection=1,butterflyDirection=1;
static uint8_t jump;
static int previousX[4],previousY[4];
static void resetGame(void){
 for(int i=0;i<5;i++)collectedWood[i]=0;
 for(int i=0;i<3;i++)collectedFlower[i]=0;
 for(int i=0;i<4;i++){previousX[i]=-20;previousY[i]=-20;}
 wood=house=flowers=phase=invincible=hint=jump=0;score=bumps=0;tick=0;
 playerX=4;playerY=101;slimeX=90;rabbitX=12;butterflyX=55;
 slimeDirection=-1;rabbitDirection=1;butterflyDirection=1;
}
static int near(int a,int b){return a-b<=9&&b-a<=9;}
static void drawHouse(void){
 // Five visible construction stages: walls, window, roof, door, heart.
 ST7735_FillRect(80,51,38,37,SKY);
 if(house==0){
  for(int x=83;x<116;x+=4){ST7735_FillRect(x,59,2,2,INK);ST7735_FillRect(x,85,2,2,INK);}
  for(int y=59;y<87;y+=4){ST7735_FillRect(83,y,2,2,INK);ST7735_FillRect(114,y,2,2,INK);}
 }
 if(house>=1)ST7735_FillRect(85,64,28,23,CREAM);
 if(house>=2){ST7735_FillRect(88,68,7,7,0x049F);ST7735_FillRect(90,70,3,3,YELLOW);}
 if(house>=3)for(int y=0;y<12;y++)ST7735_FillRect(99-y,52+y,2*y+1,1,RED);
 if(house>=4)ST7735_FillRect(101,74,8,13,0xA2A0);
 if(house>=5)heart(99,57,PINK);
}
static void objects(void){
 ST7735_FillRect(0,110,128,10,GRASS);
 for(int i=0;i<5;i++)if(!collectedWood[i]){
  ST7735_FillRect(woodX[i],111,8,7,0xA2A0);ST7735_DrawFastVLine(woodX[i]+2,112,5,YELLOW);
 }
 if(phase==2)for(int i=0;i<3;i++)if(!collectedFlower[i]){
  ST7735_DrawFastVLine(flowerX[i]+2,114,5,INK);heart(flowerX[i],110,PINK);
 }
}
static void hud(void){
 ST7735_FillRect(0,120,128,40,CREAM);
 label(2,122,"PTS",INK,2);digits(43,122,score,2);
 if(phase==0){label(2,140,word("MINE: WOOD","MINA: MADERA"),INK,1);digits(104,140,wood,1);label(110,140,"/5",INK,1);}
 if(phase==1){label(2,140,word("BUILD: HOME","CREA: CASA"),INK,1);digits(104,140,house,1);label(110,140,"/5",INK,1);}
 if(phase==2){label(2,140,word("MINE: FLOWERS","MINA: FLORES"),INK,1);digits(104,140,flowers,1);label(110,140,"/3",INK,1);}
 if(hint)label(2,151,word("STAND NEAR THE ITEM","ACERCATE AL OBJETO"),INK,1);
 else label(2,151,word("UP:JUMP DOWN:HINT","ARR:SALTO ABA:PISTA"),INK,1);
}
static void backdrop(void){
 ST7735_FillScreen(SKY);label(7,5,word("A LITTLE HOME FOR","UN HOGAR PARA"),INK,1);
 label(34,18,nickname,INK,2);
 ST7735_FillRect(9,44,19,3,0xFFFF);ST7735_FillRect(15,41,10,3,0xFFFF);
 ST7735_FillRect(61,50,12,12,YELLOW);
 ST7735_FillRect(14,66,5,22,0xA2A0);
 ST7735_FillRect(3,55,26,15,PINK);ST7735_FillRect(7,51,18,5,PINK);
 heart(8,58,RED);heart(21,61,YELLOW);
 ST7735_FillRect(0,110,128,10,GRASS);drawHouse();objects();hud();
}
static void draw(void){
 // Erase previous sprite rectangles before drawing ANY new sprite.
 // Actor lanes are clear sky, separate from the house and tree: no tile cache required.
 for(int i=0;i<4;i++)if(previousX[i]>=0)ST7735_FillRect(previousX[i],previousY[i],8,8,SKY);
 int x[4]={playerX,slimeX,rabbitX,butterflyX};
 int y[4]={playerY,101,91,35};
 if(tick%30>=15)y[3]=37;
 if(!invincible||tick%4<2)sprite8(x[0],y[0],minerArt,0);
 sprite8(x[1],y[1],slimeArt,1);sprite8(x[2],y[2],rabbitArt,2);sprite8(x[3],y[3],butterflyArt,3);
 for(int i=0;i<4;i++){previousX[i]=x[i];previousY[i]=y[i];}
}
static void iceCream(int x,int y,uint16_t scoop){
 for(int row=0;row<9;row++)ST7735_FillRect(x+row/2,y+9+row,9-row,1,0xD4A6);
 ST7735_FillRect(x-1,y+2,11,7,scoop);ST7735_FillRect(x+1,y,7,3,scoop);
 ST7735_DrawPixel(x+3,y+2,0xFFFF);
}
static void ending(void){
 ST7735_FillScreen(CREAM);label(34,6,nickname,INK,2);
 label(10,28,word("OUR HOME IS READY","NUESTRO HOGAR LISTO"),INK,1);
 label(10,44,word("LET'S GET","VAMOS POR"),INK,2);
 label(4,63,word("ICE CREAM!","UN HELADO!"),INK,2);
 // Two characters, pink and blue, sharing the ending.
 sprite8(43,105,minerArt,0);sprite8(77,105,minerArt,0);
 ST7735_FillRect(78,109,6,3,0x049F);
 iceCream(40,83,PINK);iceCream(76,83,YELLOW);heart(61,93,RED);
 label(4,122,"PTS",INK,1);digits(29,122,score,1);
 label(4,137,word("MADE FOR YOU - ARYA","PARA TI - ARYA"),INK,1);
 label(4,151,word("MINE: PLAY AGAIN","MINA: OTRA VEZ"),INK,1);
}
static void update(uint32_t held,uint32_t edges,uint32_t adc){
 (void)held;tick++;
 if(invincible)invincible--;if(hint)hint--;
 if(edges&DOWN)hint=90;
 if(tick%2==0){if(adc<1450&&playerX>1)playerX-=2;else if(adc>2650&&playerX<119)playerX+=2;}
 if((edges&UP)&&jump==0)jump=20;
 playerY=101;
 if(jump){int height;if(jump>10)height=(20-jump)*2;else height=jump*2;playerY-=height;jump--;}
 if(tick%4==0){slimeX+=slimeDirection;if(slimeX<=2)slimeDirection=1;if(slimeX>=118)slimeDirection=-1;}
 if(tick%8==0){rabbitX+=rabbitDirection;if(rabbitX<=2)rabbitDirection=1;if(rabbitX>=118)rabbitDirection=-1;}
 if(tick%6==0){butterflyX+=butterflyDirection;if(butterflyX<=36)butterflyDirection=1;if(butterflyX>=110)butterflyDirection=-1;}
 if(!invincible&&near(playerX,slimeX)&&playerY>95){
  invincible=45;bumps++;if(score>=5)score-=5;
  if(playerX<slimeX){playerX-=8;if(playerX<0)playerX=0;}else{playerX+=8;if(playerX>120)playerX=120;}
  Sound_Explosion();hud();
 }
 if((edges&MINE)&&playerY>=97){
  if(phase==0){for(int i=0;i<5;i++)if(!collectedWood[i]&&near(playerX,woodX[i])){
   collectedWood[i]=1;wood++;score+=10;Sound_Shoot();break;
  }if(wood==5)phase=1;}
  else if(phase==2){for(int i=0;i<3;i++)if(!collectedFlower[i]&&near(playerX,flowerX[i])){
   collectedFlower[i]=1;flowers++;score+=20;Sound_Shoot();break;
  }}objects();hud();
 }
 if((edges&BUILD)&&phase==1){
  if(playerX>=80&&playerY>=97){house++;wood--;score+=20;Sound_Killed();if(house==5)phase=2;drawHouse();objects();}
  else hint=90;hud();
 }
 if(edges&DOWN)hud();if(hint==0 && tick%30==0)hud();
 if(flowers==3){score+=100;screen=ENDING;Sound_Highpitch();ending();}
}
static void menu(void){
 ST7735_FillScreen(CREAM);label(22,9,"MYCRAFT",INK,2);
 label(34,31,word("FOR MUCHI","PARA MUCHI"),INK,1);
 heart(8,13,RED);heart(115,13,PINK);
 ST7735_FillRect(7,51,114,27,INK);label(16,58,word("ENGLISH","ESPANOL"),0xFFFF,2);
 label(10,91,word("UP/DOWN: LANGUAGE","ARR/ABA: IDIOMA"),INK,1);
 label(16,108,word("MINE: GO","MINA: IR"),INK,2);
 iceCream(20,136,PINK);iceCream(94,136,YELLOW);label(39,141,word("FOR YOU","PARA TI"),INK,1);
}
static void instructions(void){
 ST7735_FillScreen(CREAM);label(4,6,word("BUILD OUR HOME","CREAMOS UN HOGAR"),INK,1);
 if(!helpPage){
  label(4,25,word("SLIDE POT","POT"),INK,2);label(4,44,word("LEFT / RIGHT","IZQUIERDA / DERECHA"),INK,1);
  label(4,65,word("UP: JUMP","ARR:SALTO"),INK,2);label(4,84,word("HOP OVER THE SLIME","SALTA SOBRE LA BABA"),INK,1);
  label(4,105,word("DOWN:HINT","ABA:PISTA"),INK,2);
 }else{
  label(4,25,word("1. MINE","1. MINA"),INK,2);label(4,44,word("COLLECT 5 WOOD","RECOGE 5 MADERAS"),INK,1);
  label(4,64,word("2. BUILD","2. CREA"),INK,2);label(4,83,word("5 PRESSES BY HOUSE","5 TOQUES EN LA CASA"),INK,1);
  label(4,103,word("3. MINE","3. MINA"),INK,2);label(4,122,word("PICK 3 FLOWERS","RECOGE 3 FLORES"),INK,1);
 }
 label(4,147,word("MINE: CONTINUE","MINA: CONTINUAR"),INK,1);
}
static void handle(uint32_t held,uint32_t edges,uint32_t adc){
 switch(screen){
 case MENU:if(edges&(UP|DOWN)){lang=1-lang;menu();}if(edges&MINE){screen=HELP;helpPage=0;instructions();}break;
 case HELP:if(edges&MINE){if(!helpPage){helpPage=1;instructions();}else{resetGame();screen=PLAY;backdrop();}}break;
 case PLAY:update(held,edges,adc);if(screen==PLAY)draw();break;
 case ENDING:if(edges&MINE){screen=MENU;menu();}break;
 }
}
int main5(void){
 __disable_irq();Clock_Init_HFXT_40_80MHz(0);LaunchPad_Init();
 Switch_Init();ADCinit();Sound_Init();ST7735_InitR(MYCRAFT_LCD_TAB);menu();
 TimerG12_IntArm(CPU_HZ/30,2);__enable_irq();
 while(1){
  if(ready){
   uint32_t a,b,e;
   // Main enters here with interrupts enabled. Only copy/clear; never draw here.
   __disable_irq();a=adcSample;b=buttons;e=presses;presses=0;ready=0;__enable_irq();
   handle(b,e,a);
  }
 }
}
static void testInit(void){
 __disable_irq();Clock_Init_HFXT_40_80MHz(0);LaunchPad_Init();
 ST7735_InitR(MYCRAFT_LCD_TAB);ST7735_FillScreen(0xFF9C);
}
int main0(void){ // ADC only: move the pot through its full travel.
 testInit();ADCinit();str(1,1,"ADC PB18: 0..4095",0x294A);
 for(;;){ST7735_FillRect(0,40,128,16,0xFF9C);number(2,5,ADCin());Clock_Delay1ms(100);}
}
int main1(void){ // All text screens in both languages, no game timer.
 testInit();
 for(;;)for(lang=0;lang<2;lang++){
  menu();Clock_Delay1ms(2500);helpPage=0;instructions();Clock_Delay1ms(4000);helpPage=1;instructions();Clock_Delay1ms(4000);
  resetGame();backdrop();draw();Clock_Delay1ms(2500);
  ending();Clock_Delay1ms(2500);ending();Clock_Delay1ms(2500);
 }
}
int main2(void){ // All tile types and four sprites, animated in isolation.
 testInit();
 for(uint32_t frame=0;;frame++){
  ST7735_FillScreen(0xC73F);str(1,1,"ART TEST",0x294A);
  iceCream(14,30,PINK);iceCream(80,30,YELLOW);
  int x=8+(frame%40);
  sprite8(x,60,minerArt,0);sprite8(x+20,80,rabbitArt,2);
  sprite8(x+40,100,butterflyArt,3);sprite8(x+60,120,slimeArt,1);
  Clock_Delay1ms(100);
 }
}
int main3(void){ // Raw switch bits plus on-board PB27 LED output.
 testInit();Switch_Init();LED_Init();str(1,1,"BUTTON / LED TEST",0x294A);
 str(1,4,"UP=1 DOWN=2",0x294A);str(1,6,"MINE=4 BUILD=8",0x294A);
 for(;;){uint32_t b=Switch_In();
  if(b)LED_On(1);else LED_Off(1);
  ST7735_FillRect(0,80,128,16,0xFF9C);number(2,10,b);Clock_Delay1ms(50);
 }
}
int main4(void){ // One sound per fresh button press; SysTick only.
 testInit();Switch_Init();Sound_Init();
 str(1,1,"SOUND TEST",0x294A);str(1,4,"UP: mine",0x294A);
 str(1,6,"DOWN: build",0x294A);str(1,8,"MINE: hurt",0x294A);str(1,10,"BUILD: celebrate",0x294A);
 __enable_irq();uint32_t prev=0;
 for(;;){uint32_t now=Switch_In(),edges=now&~prev;prev=now;
  if(edges&UP)Sound_Shoot();if(edges&DOWN)Sound_Killed();
  if(edges&MINE)Sound_Explosion();if(edges&BUILD)Sound_Highpitch();
  Clock_Delay1ms(50);
 }
}
int main(void){
#if MYCRAFT_TEST_MODE == 0
 return main0();
#elif MYCRAFT_TEST_MODE == 1
 return main1();
#elif MYCRAFT_TEST_MODE == 2
 return main2();
#elif MYCRAFT_TEST_MODE == 3
 return main3();
#elif MYCRAFT_TEST_MODE == 4
 return main4();
#elif MYCRAFT_TEST_MODE == 5
 return main5();
#else
#error MYCRAFT_TEST_MODE must be 0 through 5
#endif
}
