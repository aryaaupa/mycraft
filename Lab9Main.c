// Lab9Main.c
// Runs on MSPM0G3507
// Lab 9 ECE319K
// Arya Upadhyay
// Last Modified: November 21, 2026

#include <stdio.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/ADC1.h"
#include "../inc/DAC5.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/images.h"
// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz
void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
   // Clock_Init80MHz(0);   // run this line for 80MHz
  Clock_Init_HFXT_40_80MHz(0);
}

// Spanish replaces the original Arabic output demonstration.
// Keep main0 as a separate LCD/language test; main() runs MYCRAFT.
int main0(void){ // main 0, demonstrate Spanish output
  Clock_Init80MHz(0);
  LaunchPad_Init();
  ST7735_InitR(INITR_REDTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(ST7735_WHITE);
  ST7735_SetTextColor(ST7735_BLACK);
  ST7735_SetCursor(0, 1);
  ST7735_OutString("Hola!");
  ST7735_SetCursor(0, 3);
  ST7735_OutString("Bienvenido a MYCRAFT");
  ST7735_SetCursor(0, 5);
  ST7735_OutString("Recolecta recursos");
  ST7735_SetCursor(0, 7);
  ST7735_OutString("Construye y gana!");
  while(1){
  }
}
uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}


// games  engine runs at 30Hz
static volatile uint32_t pending=0;
static volatile uint32_t inputLatched=0;
void TIMG12_IRQHandler(void){uint32_t pos,msg;
  if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
// game engine goes here
    // 1) sample slide pot
    // 2) read input switches
    // 3) move sprites
    // 4) start sounds
    // 5) set semaphore
    if(pending<3) pending++;
    // Latch any button press even when main is busy drawing LCD tiles.
    inputLatched |= Switch_In();
    // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}
uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[] ="All\x83";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Spanish[]="Adi\xA2s";
const char Goodbye_Portuguese[] = "Tchau";
const char Goodbye_French[] = "Au revoir";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char Language_Portuguese[]="Portugu\x88s";
const char Language_French[]="Fran\x87" "ais";
const char *Phrases[3][4]={
  {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French}
};
// use main1 to observe special characters
int main1(void){ // main1
    char l;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf(INITR_REDTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(0x0000);            // set screen to black
  for(phrase_t myPhrase=HELLO; myPhrase<= GOODBYE; myPhrase++){
    for(Language_t myL=English; myL<= French; myL++){
         ST7735_OutString((char *)Phrases[LANGUAGE][myL]);
      ST7735_OutChar(' ');
         ST7735_OutString((char *)Phrases[myPhrase][myL]);
      ST7735_OutChar(13);
    }
  }
  Clock_Delay1ms(3000);
  ST7735_FillScreen(0x0000);       // set screen to black
  l = 128;
  while(1){
    Clock_Delay1ms(2000);
    for(int j=0; j < 3; j++){
      for(int i=0;i<16;i++){
        ST7735_SetCursor(7*j+0,i);
        ST7735_OutUDec(l);
        ST7735_OutChar(' ');
        ST7735_OutChar(' ');
        ST7735_SetCursor(7*j+4,i);
        ST7735_OutChar(l);
        l++;
      }
    }
  }
}

// use main2 to observe graphics
// Original starter does not define main2(); comment preserved.

// MYCRAFT gameplay implementation (original classroom demonstrations remain below)
#define W 32
#define H 19
#define TILE 8
#define UP 1u
#define DOWN 2u
#define MINE 4u
#define BUILD 8u
#define AIR 0
#define DIRT 1
#define STONE 2
#define WOOD 3
#define LEAF 4
#define BRICK 5
#define ORE 6
#define SKY 0x7DFF
#define BROWN 0xA2A0
#define GREY 0x8410
#define GOLD 0x07FF
#define PINK 0xF81F
static uint8_t world[H][W];
static uint8_t lang=0, state=0, inventory=0, built=0, hp=3;
static uint16_t score=0;
static int playerX=4,playerY=8, enemyX=21, enemyY=8, rabbitX=10, butterflyX=15;
static uint32_t tick=0;
// Original starter code: uint32_t M=1; Random32() and Random(n)
static uint32_t seed=0x1c44d5u;
static uint32_t random32(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static const char *word(const char *en,const char *es){return lang?es:en;}
static void str(int x,int y,const char *s,uint16_t c){ST7735_SetTextColor(c);ST7735_SetCursor(x,y);ST7735_OutString((char*)s);}
static void number(int x,int y,uint32_t n){ST7735_SetCursor(x,y);ST7735_OutUDec(n);}
// Minecraft-style survival: mine materials, collect diamonds, build a shelter.
// The potentiometer moves horizontally; UP jumps, DOWN crouches/digs downward.
static uint8_t diamonds=0, dayPhase=0, facing=1, jumpFrames=0;
static uint8_t mineFlash=0;
static uint8_t inputStable=0, inputCandidate=0, inputCount=0;

static uint8_t stock[4]={0,0,0,0}; // dirt, stone, wood, leaves
static uint8_t selected=0, beaconReady=0, menuChoice=0;
static const uint8_t materials[4]={DIRT,STONE,WOOD,LEAF};
static const char *matName[4]={"DIRT","STONE","WOOD","LEAF"};
static uint8_t materialSlot(uint8_t m){
 switch(m){case DIRT:return 0;case STONE:return 1;case WOOD:return 2;default:return 3;}
}
static void heart(int x,int y,uint16_t c){
 ST7735_DrawPixel(x,y,c);ST7735_DrawPixel(x+2,y,c);
 ST7735_FillRect(x,y+1,3,2,c);ST7735_DrawPixel(x+1,y+3,c);
}
static void worldInit(void){
 int x,y;
 for(y=0;y<H;y++)for(x=0;x<W;x++){
   if(y<11)world[y][x]=AIR;
   else if(y==11)world[y][x]=DIRT;
   else if(y<14)world[y][x]=(random32()%5==0)?STONE:DIRT;
   else world[y][x]=(random32()%12==0)?ORE:((random32()%3==0)?DIRT:STONE);
 }
 // A playable cave entrance and a small tunnel.
 for(y=11;y<17;y++)for(x=6;x<9;x++)world[y][x]=AIR;
 for(x=8;x<15;x++)world[15][x]=AIR;
 // Oak trees.
 for(y=7;y<11;y++)world[y][18]=WOOD;
 for(y=5;y<8;y++)for(x=16;x<21;x++)if((x+y)%5!=0)world[y][x]=LEAF;
 for(y=8;y<11;y++)world[y][27]=WOOD;
 for(y=6;y<9;y++)for(x=25;x<30;x++)if((x+y)%5!=0)world[y][x]=LEAF;
 // Visible diamond ore, plus underground ore.
 world[13][11]=ORE;world[14][19]=ORE;world[16][25]=ORE;
 playerX=4;playerY=10;enemyX=23;enemyY=10;rabbitX=12;butterflyX=15;
 inventory=0;built=0;diamonds=0;score=0;hp=3;tick=0;selected=0;beaconReady=0;for(x=0;x<4;x++)stock[x]=0;
 dayPhase=0;facing=1;jumpFrames=0;mineFlash=0;
}
// MYCRAFT 8x8 pixel-art renderer. Original game controls and logic preserved.
// All drawing stays in main context, never in TIMG12_IRQHandler.
#define RGB565(r,g,b) (uint16_t)((((r)&31u)<<11)|(((g)&63u)<<5)|((b)&31u))
static uint16_t color(uint8_t t){
 switch(t){
 case DIRT:return 0x9A64;  // earthy brown
 case STONE:return 0x8410; // cobblestone gray
 case WOOD:return 0x8A22;  // oak log
 case LEAF:return 0x2D45;  // forest foliage
 case BRICK:return 0xB3AA; // placed brick
 case ORE:return 0x6B4D;   // stone containing diamond
 default:return 0x7DFF;    // bright sky
 }
}
// Texture detail is deterministic: no random changes between frames.
static void tileArt(int sx,int sy,int wx,int wy,uint8_t t){
 uint32_t h=(uint32_t)(wx*173+wy*337+41);
 uint16_t base=color(t);
 int k;
 if(t==AIR){
   ST7735_FillRect(sx,sy,8,8,base);
   // Sparse square-edged clouds, without extra RAM or bitmaps.
   if(wy==3 && ((wx/4)%5)==1){
     ST7735_FillRect(sx,sy+4,8,2,0xFFFF);
   }
   return;
 }
 if(t==DIRT && wy==11){
   ST7735_FillRect(sx,sy,8,8,0x9A64);
   ST7735_FillRect(sx,sy,8,3,0x2DA5);
   ST7735_DrawFastHLine(sx,sy,8,0x4F26);
   ST7735_DrawPixel(sx+2,sy+3,0x2DA5);
   ST7735_DrawPixel(sx+6,sy+3,0x2DA5);
 }else{
   ST7735_FillRect(sx,sy,8,8,base);
 }
 if(t==DIRT){
   for(k=0;k<5;k++){
     int px=(int)((h+k*11u)%7u),py=(int)(((h>>3)+k*7u)%5u)+3;
     ST7735_DrawPixel(sx+px,sy+py,(k&1)?0x71C3:0xB2E5);
   }
 }else if(t==STONE || t==ORE){
   ST7735_DrawFastHLine(sx+1,sy+2,4,0xA514);
   ST7735_DrawFastVLine(sx+5,sy+3,4,0x5AEB);
   ST7735_DrawPixel(sx+2,sy+6,0xBDF7);
   if(t==ORE){
     ST7735_FillRect(sx+2,sy+3,3,2,0x05FF);
     ST7735_DrawPixel(sx+3,sy+2,0xBFFF);
     ST7735_DrawPixel(sx+5,sy+5,0x05FF);
   }
 }else if(t==WOOD){
   ST7735_DrawFastVLine(sx+1,sy,8,0xB3A5);
   ST7735_DrawFastVLine(sx+5,sy,8,0x61A1);
   ST7735_DrawPixel(sx+3,sy+3,0xD4A6);
 }else if(t==LEAF){
   ST7735_FillRect(sx+1,sy+1,3,2,0x3EA6);
   ST7735_DrawPixel(sx+6,sy+5,0x1BA3);
   ST7735_DrawPixel(sx+3,sy+6,0x5F07);
 }else if(t==BRICK){
   ST7735_DrawFastHLine(sx,sy+3,8,0x6A69);
   ST7735_DrawFastVLine(sx+4,sy,3,0x6A69);
   ST7735_DrawFastVLine(sx+2,sy+4,4,0x6A69);
   ST7735_DrawFastHLine(sx+1,sy+1,3,0xD4AE);
 }
 ST7735_DrawFastHLine(sx,sy+7,8,0x3186);
 ST7735_DrawFastVLine(sx+7,sy,8,0x3186);
}
// Palette-indexed 8x8 character sprites. 0 is transparent.
// Each hex digit is one pixel: hair, skin, blue shirt, denim, boots, eyes.
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
   switch(n){case '3':return 0x4F26;case '6':return 0x0000;case '4':return 0x1B42;default:return 0xFFFF;}
 }
 if(kind==2){ // white rabbit
   switch(n){case '2':return 0xFFFF;case '6':return 0xF9B6;default:return 0xFFFF;}
 }
 if(kind==3){ // butterfly
   switch(n){case '3':return 0xF99F;case '4':return 0x4208;default:return 0xFFFF;}
 }
 switch(n){case '1':return 0x39C7;case '2':return 0xFCAE;
 case '3':return 0x049F;case '4':return 0xFCAE;
 case '5':return 0x21A9;case '6':return 0x0000;default:return 0xFFFF;}
}
static void sprite8(int sx,int sy,const char art[8][9],uint8_t kind){
 int x,y;
 for(y=0;y<8;y++)for(x=0;x<8;x++){
   char n=art[y][x];
   if(n!='0')ST7735_DrawPixel(sx+x,sy+y,spriteColor(n,kind));
 }
}
static void person(int x,int y,uint16_t shirt){
 (void)shirt;
 sprite8(x*8,(y-2)*8,minerArt,0);
}
static void draw(void){
 int x,y,cam=playerX-8;
 if(cam<0)cam=0;
 if(cam>W-16)cam=W-16;
 for(y=2;y<17;y++)for(x=0;x<16;x++){
   int wx=x+cam;
   uint8_t t=world[y][wx];
   tileArt(x*8,(y-2)*8,wx,y,t);
   if(t==AIR && dayPhase && y<11)
     ST7735_DrawFastHLine(x*8,(y-2)*8,8,0x4210);
   if(t==AIR && y==10 && (wx%11)==2){
     ST7735_DrawPixel(x*8+3,(y-2)*8+6,0xF99F); // flowers
     ST7735_DrawPixel(x*8+3,(y-2)*8+7,0x07E0);
   }
 }
 person(playerX-cam,playerY,0x001F);
 if(enemyX>=cam&&enemyX<cam+16)
   sprite8((enemyX-cam)*8,(enemyY-2)*8,slimeArt,1);
 if(rabbitX>=cam&&rabbitX<cam+16)
   sprite8((rabbitX-cam)*8,8*8,rabbitArt,2);
 if(butterflyX>=cam&&butterflyX<cam+16)
   sprite8((butterflyX-cam)*8,32,butterflyArt,3);
 if(mineFlash){
   int px=(playerX-cam)*8+(facing?8:-3),py=(playerY-2)*8+2;
   ST7735_FillRect(px,py,3,3,0xFFE0);
 }
 ST7735_FillRect(0,120,128,40,0x1082);
 for(x=0;x<3;x++)heart(3+x*8,122,x<hp?0xF99F:0x4208);
 str(5,15,dayPhase?"NIGHT":"DAY",0xFFE0);
 str(13,15,"G:",0x07FF);number(16,15,diamonds);
 // Four colored hotbar slots and selected outline.
 for(x=0;x<4;x++){
   int px=3+x*31;
   ST7735_FillRect(px,130,28,16,x==selected?0xFFE0:0x6B4D);
   ST7735_FillRect(px+2,132,24,12,0x1082);
   ST7735_FillRect(px+4,135,7,7,color(materials[x]));
   ST7735_SetTextColor(0xFFFF);ST7735_SetCursor((px+12)/6,17);
   ST7735_OutUDec(stock[x]);
 }
 str(0,18,word("DN:ITEM  MINE/BUILD","ABA:ITEM MIN/CRE"),0xFFFF);
 str(0,19,"PTS:",0xFFE0);number(5,19,score);
}
static void menu(void){
 int x;
 ST7735_FillScreen(0x7DFF);
 ST7735_FillRect(0,110,128,50,0x2DA5);
 ST7735_FillRect(0,121,128,39,0x9A64);
 for(x=0;x<128;x+=21){
   ST7735_FillRect(x+3,104,13,6,0xFFFF);
   ST7735_FillRect(x+7,101,9,3,0xFFFF);
 }
 ST7735_FillRect(96,15,14,14,0xFFE0);
 heart(18,18,0xF99F);heart(109,51,0xF99F);
 str(3,2,"MYCRAFT",0xF81F);
 str(2,4,"COZY EDITION",0x0000);
 ST7735_FillRect(10,55,108,25,0xFFFF);
 str(3,7,lang?"> ESPANOL":"> ENGLISH",0x001F);
 str(1,11,word("UP/DN LANGUAGE","ARR/ABA IDIOMA"),0x0000);
 str(2,13,"MINE: START",0x0000);
 str(1,18,"MADE WITH LOVE",0xFFFF);
}
static void endScreen(uint8_t win){
 int i;
 ST7735_FillScreen(win?0x7DFF:0x0000);
 for(i=0;i<9;i++)heart((i*13)%124,8+(i*17)%100,0xF99F);
 str(1,5,win?word("HOME IS BUILT!","HOGAR LISTO!"):word("GAME OVER","FIN DEL JUEGO"),win?0x001F:PINK);
 str(2,8,word("SCORE:","PUNTOS:"),win?0x0000:0xFFFF);number(10,8,score);
 str(1,12,word("MADE WITH LOVE","HECHO CON AMOR"),win?0xF81F:0xFFFF);
 str(1,16,word("MINE: RESTART","MINA: REINICIA"),win?0x0000:GOLD);
}
static void update(uint32_t buttons,uint32_t edges,uint32_t adc){
 int nx=playerX,tx,ty;
 if(tick%3==0){
   if(adc<1450){nx--;facing=0;}
   else if(adc>2650){nx++;facing=1;}
 }
 if(nx>=0&&nx<W&&world[playerY][nx]==AIR)playerX=nx;
 if((edges&UP)&&jumpFrames==0&&playerY>2)jumpFrames=4;
 if(jumpFrames){
   if(playerY>2&&world[playerY-1][playerX]==AIR)playerY--;
   jumpFrames--;
 }else if(tick%3==0&&playerY<H-1&&world[playerY+1][playerX]==AIR)playerY++;
 if(edges&DOWN){selected=(selected+1)%4;Sound_Highpitch();}
 if(edges&MINE){
   tx=playerX+(facing?1:-1);ty=playerY;
   // Mine forward first, then the tile below when forward is empty.
   if(tx>=0&&tx<W&&world[ty][tx]==AIR){tx=playerX;ty=playerY+1;}
   if(tx>=0&&tx<W&&ty<H&&world[ty][tx]!=AIR){
     uint8_t material=world[ty][tx];
     world[ty][tx]=AIR;
     if(material==ORE){if(diamonds<99)diamonds++;score+=40;Sound_Highpitch();}
     else{
       uint8_t slot=materialSlot(material);
       if(stock[slot]<99)stock[slot]++;
       score+=10;Sound_Shoot();
     }
     mineFlash=2;
   }else if(enemyX>=playerX-1&&enemyX<=playerX+1
             &&enemyY>=playerY-1&&enemyY<=playerY+1){
     enemyX=29;score+=20;Sound_Killed();
   }
 }
 if((edges&BUILD)&&stock[selected]){
   tx=playerX+(facing?1:-1);
   if(tx>=0&&tx<W&&world[playerY][tx]==AIR&&tx!=enemyX){
     world[playerY][tx]=materials[selected];
     stock[selected]--;built++;score+=15;Sound_Highpitch();
   }
 }
 if(mineFlash)mineFlash--;
 if(tick%15==0){
   if(rabbitX>0&&rabbitX<W-1)rabbitX+=(tick%30==0)?1:-1;
   butterflyX+=(tick%60<30)?1:-1;
   if(butterflyX<1)butterflyX=1;
   if(butterflyX>=W-1)butterflyX=W-2;
 }
 dayPhase=(tick>=1800);
 if(tick%(dayPhase?18:45)==0){
   if(enemyX>playerX)enemyX--;else if(enemyX<playerX)enemyX++;
   if(enemyX==playerX&&(enemyY==playerY||enemyY==playerY+1)){
     if(hp)hp--;Sound_Explosion();
   }
 }
 // Build five blocks and find three crystals to activate the home beacon.
 if(diamonds>=3&&built>=5){
   state=2;score+=100;Sound_Highpitch();endScreen(1);
 }else if(!hp){state=3;endScreen(0);}
 (void)buttons;
}

int main(void){
 uint32_t last=0,buttons,edges,adc,latched;
 __disable_irq();
 PLL_Init();LaunchPad_Init();Switch_Init();ADCinit();Sound_Init();
 ST7735_InitR(INITR_REDTAB);
 menu();TimerG12_IntArm(80000000u/30u,2);
 __enable_irq();
 while(1){
   if(!pending)continue;
   __disable_irq();pending--;latched=inputLatched;inputLatched=0;__enable_irq();
   buttons=Switch_In()|latched;
   // 30 Hz input sampling, with held input and edge-triggered actions.
   edges=buttons&~last;last=buttons;
   adc=ADCin();
   if(state==0){
     if(edges&(UP|DOWN)){lang^=1;menu();}
     if(edges&(MINE|BUILD)){worldInit();state=1;Sound_Highpitch();draw();}
   }else if(state==1){
     tick++;update(buttons,edges,adc);
     // Refresh at 5 Hz or after an action; reduces SPI work.
     if(state==1&&(tick%6==0||edges))draw();
   }else if(edges&MINE){state=0;menu();}
 }
}


#if 0
// Original main() preserved for reference; disabled to avoid two entry points.
int main(void){
  __disable_irq();

PLL_Init();
LaunchPad_Init();
Switch_Init();
ST7735_InitPrintf(INITR_REDTAB);

  // Background
  ST7735_FillScreen(ST7735_GREEN);

  // Game title
  ST7735_FillRect(0, 0, 128, 20, ST7735_BLACK);
  ST7735_SetCursor(1, 0);
  ST7735_OutString("MYCRAFT");

  // Score
  ST7735_SetCursor(1, 2);
  ST7735_OutString("Score: 0");

  // A tree (trunk + leaves)
  ST7735_FillRect(85, 55, 8, 22, 0x8200);
  ST7735_FillRect(76, 38, 26, 23, 0x0320);

  // Resource block
  ST7735_FillRect(25, 95, 16, 16, 0xFD20);

  // Player: head
  ST7735_FillRect(54, 68, 12, 12, 0xFE10);

  // Player: body
  ST7735_FillRect(52, 80, 16, 16, ST7735_BLUE);

  // Player: legs
  ST7735_FillRect(53, 96, 6, 10, ST7735_BLACK);
  ST7735_FillRect(61, 96, 6, 10, ST7735_BLACK);

  uint32_t buttons;
uint32_t lastButtons = 0;

while(1){
  buttons = Switch_In();

  if(buttons != lastButtons){
    ST7735_SetCursor(1, 3);
    ST7735_OutString("Buttons: ");
    ST7735_OutUDec(buttons);
    ST7735_OutString("   ");
    lastButtons = buttons;
  }

  Clock_Delay1ms(20);
}
}


#endif

// use main3 to test switches and LEDs
int main3(void){ // main3
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  while(1){
    // write code to test switches and LEDs
    
  }
}
// use main4 to test sound outputs
int main4(void){ uint32_t last=0,now;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  Sound_Init();  // initialize sound
  TExaS_Init(ADC0,6,0); // ADC1 channel 6 is PB20, TExaS scope
  __enable_irq();
  while(1){
    now = Switch_In(); // one of your buttons
    if((last == 0)&&(now == 1)){
      Sound_Shoot(); // call one of your sounds
    }
    if((last == 0)&&(now == 2)){
      Sound_Killed(); // call one of your sounds
    }
    if((last == 0)&&(now == 4)){
      Sound_Explosion(); // call one of your sounds
    }
    if((last == 0)&&(now == 8)){
      Sound_Fastinvader1(); // call one of your sounds
    }
    // modify this to test all your sounds
  }
}

// ALL ST7735 OUTPUT MUST OCCUR IN MAIN
int main5(void){ // final main
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf(INITR_REDTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(ST7735_BLACK);
  ADCinit();     //PB18 = ADC1 channel 5, slidepot
  Switch_Init(); // initialize switches
  LED_Init();    // initialize LED
  Sound_Init();  // initialize sound
  TExaS_Init(0,0,&TExaS_LaunchPadLogicPB27PB26); // PB27 and PB26
    // initialize interrupts on TimerG12 at 30 Hz
  TimerG12_IntArm(80000000/30,2);
  // initialize all data structures
  __enable_irq();

  while(1){
    // wait for semaphore
       // clear semaphore
       // update ST7735R
    // check for end game or level switch
  }
}