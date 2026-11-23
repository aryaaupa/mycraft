// MyCraft Cozy - replace Lab9Main.c in your existing CCS Lab 9 project.
// Based on Arya Upadhyay's uploaded implementation; see README for integration.
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/Timer.h"
#include "../inc/ADC1.h"
#include "Switch.h"
#include "Sound.h"
#include "LED.h"
#define CPU_HZ 80000000u
#ifndef MYCRAFT_LCD_TAB
#define MYCRAFT_LCD_TAB INITR_REDTAB
#endif
// Ring buffer: producer is TIMG12 ISR, consumer is main. Never render in an ISR.
typedef struct { uint16_t adc; uint8_t held,edges; } Input;
static volatile Input queue[16];
static volatile uint8_t head,tail;
volatile uint32_t MyCraft_DroppedInputs;
static uint8_t stableButtons,debounce[4];
void TIMG12_IRQHandler(void){
 if(TIMG12->CPU_INT.IIDX==1){
  uint32_t raw=Switch_In(); uint8_t edges=0;
  static uint32_t filtered; static uint8_t initialized;
  uint32_t sample=ADCin();
  if(!initialized){filtered=sample;initialized=1;}else filtered=(3*filtered+sample+2)/4;
  for(unsigned i=0;i<4;i++){
   uint8_t bit=1u<<i;
   if((raw&bit)==(stableButtons&bit))debounce[i]=0;
   else if(++debounce[i]>=2){stableButtons^=bit;debounce[i]=0;if(stableButtons&bit)edges|=bit;}
  }
  uint8_t next=(head+1)&15;
  if(next!=tail){queue[head].adc=filtered;queue[head].held=stableButtons;queue[head].edges=edges;head=next;}
  else MyCraft_DroppedInputs++;
 }
}
static int pop(Input *out){
 uint32_t key=__get_PRIMASK(); __disable_irq();
 if(tail==head){__set_PRIMASK(key);return 0;}
 out->adc=queue[tail].adc;out->held=queue[tail].held;out->edges=queue[tail].edges;
 tail=(tail+1)&15;__set_PRIMASK(key);return 1;
}
// MYCRAFT gameplay implementation (original classroom demonstrations remain below)
#define W 32
#define H 17
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
#define SKY 0xC73F
#define BROWN 0xA2A0
#define GREY 0x8410
#define GOLD 0x07FF
#define PINK 0xF81F
static uint8_t world[H][W];
static uint8_t placed[H][W];
static uint8_t cache[15][16];
static int lastCam=-1;
static uint8_t hudDirty=1;
static uint8_t lang=0, state=0, hp=3;
static uint16_t built;
static uint16_t score=0;
static int playerX=4,playerY=8, enemyX=21, enemyY=8, rabbitX=10, butterflyX=15;
static uint32_t tick=0;
// Original starter code: uint32_t M=1; Random32() and Random(n)
static uint32_t seed=0x1c44d5u;
static uint32_t random32(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static const char *word(const char *en,const char *es){return lang?es:en;}
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
   for(int col=0;col<5;col++)if(bits&(16>>col)){
    int px=x+col*scale,py=y+row*scale;
    if(px>=0&&py>=0&&px+scale<=128&&py+scale<=160)
     ST7735_FillRect(px,py,scale,scale,color);
   }
  }
 }
}
static void str(int x,int y,const char *s,uint16_t c){label(x*6,y*8,s,c,1);}
static void number(int x,int y,uint32_t n){char b[12];snprintf(b,sizeof b,"%lu",(unsigned long)n);label(x*6,y*8,b,0x294A,1);}
// Minecraft-style survival: mine materials, collect diamonds, build a shelter.
// The potentiometer moves horizontally; UP jumps, DOWN crouches/digs downward.
static uint8_t diamonds=0, dayPhase=0, facing=1, jumpFrames=0;
static uint8_t mineFlash=0;
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
 memset(placed,0,sizeof placed);
 memset(cache,255,sizeof cache); lastCam=-1; hudDirty=1;
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
 // A cherry tree visible from the starting meadow.
 for(y=7;y<11;y++)world[y][2]=WOOD;
 for(y=5;y<8;y++)for(x=0;x<5;x++)if((x+y)%5!=0)world[y][x]=LEAF;
 // Visible diamond ore, plus underground ore.
 world[10][9]=ORE;world[10][15]=ORE;world[10][24]=ORE;
 // Fixed reachable crystals; underground exploration remains optional.
 world[10][6]=WOOD;world[10][11]=WOOD;world[10][20]=WOOD;
 world[10][26]=WOOD;world[10][29]=WOOD;
 playerX=4;playerY=10;enemyX=23;enemyY=10;rabbitX=12;butterflyX=15;
 built=0;diamonds=0;score=0;hp=3;tick=0;selected=0;beaconReady=0;for(x=0;x<4;x++)stock[x]=0;
 dayPhase=0;facing=1;jumpFrames=0;mineFlash=0;
}
// MYCRAFT 8x8 pixel-art renderer. Original game controls and logic preserved.
// All drawing stays in main context, never in TIMG12_IRQHandler.
#define RGB565(r,g,b) (uint16_t)((((r)&31u)<<11)|(((g)&63u)<<5)|((b)&31u))
static uint16_t color(uint8_t t){
 switch(t){
 case DIRT:return 0xC3CA;  // earthy brown
 case STONE:return 0x8410; // cobblestone gray
 case WOOD:return 0x8A22;  // oak log
 case LEAF:return 0xFBB7;  // forest foliage
 case BRICK:return 0xB3AA; // placed brick
 case ORE:return 0x6B4D;   // stone containing diamond
 default:return 0xC73F;    // bright sky
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
   ST7735_FillRect(sx,sy,8,8,0xC3CA);
   ST7735_FillRect(sx,sy,8,3,0x8DF1);
   ST7735_DrawFastHLine(sx,sy,8,0xB6B4);
   ST7735_DrawPixel(sx+2,sy+3,0x8DF1);
   ST7735_DrawPixel(sx+6,sy+3,0x8DF1);
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
   ST7735_FillRect(sx+1,sy+1,3,2,0xFDDC);
   ST7735_DrawPixel(sx+6,sy+5,0xEAB4);
   ST7735_DrawPixel(sx+3,sy+6,0xFF3D);
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
static void person(int x,int y,uint16_t shirt){
 (void)shirt;
 sprite8(x*8,(y-2)*8,minerArt,0);
}
static void draw(void){
 int x,y,cam=playerX-8;
 if(cam<0)cam=0;
 if(cam>W-16)cam=W-16;
 if(cam!=lastCam){memset(cache,255,sizeof cache);lastCam=cam;}
 for(y=2;y<17;y++)for(x=0;x<16;x++){
   int wx=x+cam;
   uint8_t t=world[y][wx];
   if(cache[y-2][x]==t)continue;
   cache[y-2][x]=t;
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
 // Dirty cells covering actors/effects must be repainted on the next frame.
 {int ax[5]={playerX,enemyX,rabbitX,butterflyX,playerX+(facing?1:-1)};
  int ay[5]={playerY,enemyY,10,6,playerY};
  for(int i=0;i<5;i++)for(int dx=-1;dx<=1;dx++){
   int cx=ax[i]-cam+dx,cy=ay[i]-2;
   if(cx>=0&&cx<16&&cy>=0&&cy<15)cache[cy][cx]=255;
  }
 }
 if(hudDirty){
  char line[24];
  ST7735_FillRect(0,120,128,40,0x1082);
  snprintf(line,sizeof line,"PTS %u",score);
  label(2,121,line,0xFFFF,2);
  for(x=0;x<3;x++)heart(102+x*8,150,x<hp?0xFFFF:0x6B4D);
  snprintf(line,sizeof line,"GEM %u/3 %s %u/5",diamonds,word("HOME","CASA"),built);
  label(2,138,line,0xFFFF,1);
  static const char *names[2][4]={{"DIRT","STONE","WOOD","LEAVES"},{"TIERRA","PIEDRA","MADERA","HOJAS"}};
  ST7735_FillRect(2,150,7,7,color(materials[selected]));
  snprintf(line,sizeof line,"%s %u",names[lang][selected],stock[selected]);
  label(14,150,line,0xFFFF,1);
  hudDirty=0;
 }

}
static void menu(void){
 ST7735_FillScreen(0xFF9C);
 label(22,12,"MYCRAFT",0x294A,2);
 label(28,34,word("COZY GARDEN","JARDIN FELIZ"),0x294A,1);
 ST7735_FillRect(7,53,114,28,0x294A);
 label(16,60,lang?"ESPANOL":"ENGLISH",0xFFFF,2);
 label(10,92,word("UP/DOWN: LANGUAGE","ARR/ABA: IDIOMA"),0x294A,1);
 label(16,109,word("MINE: GO","MINA: IR"),0x294A,2);
 heart(7,14,0xD295);heart(117,14,0xD295);
 ST7735_FillRect(0,136,128,24,0x8DF1);
 sprite8(9,143,minerArt,0);sprite8(110,143,rabbitArt,2);
 label(20,144,word("WITH LOVE","CON AMOR"),0x294A,1);
}
static void endScreen(uint8_t win){
 char line[16];ST7735_FillScreen(0xFF9C);
 label(10,16,win?word("YOU WIN!","GANASTE!"):word("GAME OVER","FIN JUEGO"),0x294A,2);
 label(28,51,word("SCORE","PUNTOS"),0x294A,2);
 snprintf(line,sizeof line,"%u",score);label(28,74,line,0x294A,2);
 label(16,110,word("MINE: MENU","MINA: MENU"),0x294A,1);
 label(16,140,word("MADE WITH LOVE","HECHO CON AMOR"),0x294A,1);
}
static void update(uint32_t buttons,uint32_t edges,uint32_t adc){
 int nx=playerX,tx,ty;
 uint16_t oldScore=score;uint8_t oldHp=hp,oldSelected=selected;
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
     uint8_t recycled=placed[ty][tx];
     if(recycled){placed[ty][tx]=0;if(built)built--;}
     if(material==ORE){if(diamonds<99)diamonds++;score+=40;Sound_Highpitch();}
     else{
       uint8_t slot=materialSlot(material);
       if(stock[slot]<99)stock[slot]++;
       if(!recycled)score+=10;Sound_Shoot();
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
     placed[playerY][tx]=1;
     stock[selected]--;built++;Sound_Killed();
   }
 }
 if(mineFlash)mineFlash--;
 if(tick%15==0){
   if(rabbitX>0&&rabbitX<W-1)rabbitX+=(tick%30==0)?1:-1;
   butterflyX+=(tick%60<30)?1:-1;
   if(butterflyX<1)butterflyX=1;
   if(butterflyX>=W-1)butterflyX=W-2;
 }
 dayPhase=0; // A sunny garden; slime remains a gentle moving obstacle.
 if(tick%(dayPhase?18:45)==0){
   if(enemyX>playerX)enemyX--;else if(enemyX<playerX)enemyX++;
   if(enemyX==playerX&&(enemyY==playerY||enemyY==playerY+1)){
     if(hp)hp--;Sound_Explosion();
   }
 }
 // Build five blocks and find three crystals to activate the home beacon.
 if(oldScore!=score||oldHp!=hp||oldSelected!=selected||(edges&(MINE|BUILD)))hudDirty=1;
 if(diamonds>=3&&built>=5){
   state=2;score+=100;Sound_Highpitch();endScreen(1);
 }else if(!hp){state=3;endScreen(0);}
 (void)buttons;
}

static uint8_t helpPage;
static void instructions(void){
 ST7735_FillScreen(0xFF9C);
 label(4,6,word("HOW TO PLAY","COMO JUGAR"),0x294A,1);
 if(helpPage==0){
  label(4,24,word("SLIDE POT","POT"),0x294A,2);
  label(4,43,word("LEFT / RIGHT","IZQUIERDA / DERECHA"),0x294A,1);
  label(4,63,word("UP","ARRIBA"),0x294A,2);
  label(4,82,word("JUMP","SALTAR"),0x294A,1);
  label(4,102,word("DOWN","ABAJO"),0x294A,2);
  label(4,121,word("SELECT MATERIAL","ELEGIR MATERIAL"),0x294A,1);
  label(4,146,word("MINE: NEXT","MINA: SIGUIENTE"),0x294A,1);
 }else{
  label(4,24,word("MINE","MINA"),0x294A,2);
  label(4,43,word("COLLECT BLOCKS","RECOGER BLOQUES"),0x294A,1);
  label(4,63,word("BUILD","CREA"),0x294A,2);
  label(4,82,word("PLACE A BLOCK","PONER UN BLOQUE"),0x294A,1);
  label(4,104,word("GOAL","META"),0x294A,2);
  label(4,123,word("3 GEMS + 5 BLOCKS","3 GEMAS + 5 BLOQUES"),0x294A,1);
  label(4,146,word("MINE: PLAY","MINA: JUGAR"),0x294A,1);
 }
}
int main5(void){
 Input in; uint8_t redraw=0;
 __disable_irq();
 Clock_Init_HFXT_40_80MHz(0);
 LaunchPad_Init(); Switch_Init(); ADCinit(); Sound_Init();
 ST7735_InitR(MYCRAFT_LCD_TAB);
 menu();
 TimerG12_IntArm(CPU_HZ/30u,2);
 __enable_irq();
 for(;;){
  while(pop(&in)){
   if(state==0){
    if(in.edges&(UP|DOWN)){lang^=1;menu();}
    if(in.edges&MINE){state=4;helpPage=0;instructions();}
   }else if(state==4){
    if(in.edges&MINE){if(!helpPage){helpPage=1;instructions();}else{worldInit();state=1;redraw=1;}}
   }else if(state==1){
    tick++;update(in.held,in.edges,in.adc);if(state==1)redraw=1;else redraw=0;
   }else if(in.edges&MINE){state=0;menu();}
  }
  if(redraw&&state==1){draw();redraw=0;}
 }
}

// Build-time module selector. Change only this number, rebuild, and flash.
#ifndef MYCRAFT_TEST_MODE
#define MYCRAFT_TEST_MODE 5
#endif
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
  worldInit();draw();Clock_Delay1ms(2500);
  endScreen(1);Clock_Delay1ms(2500);endScreen(0);Clock_Delay1ms(2500);
 }
}
int main2(void){ // All tile types and four sprites, animated in isolation.
 testInit();
 for(uint32_t frame=0;;frame++){
  ST7735_FillScreen(0xC73F);str(1,1,"ART TEST",0x294A);
  for(int t=AIR;t<=ORE;t++)tileArt(6+t*16,30,t,11,t);
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
