#ifndef MYCRAFT_SOUND_H
#define MYCRAFT_SOUND_H
#include <stdint.h>
void Sound_Init(void);
void Sound_Start(const uint8_t *samples,uint32_t count);
void Sound_Shoot(void);
void Sound_Killed(void);
void Sound_Explosion(void);
void Sound_Highpitch(void);
void Sound_Fastinvader1(void);
void Sound_Fastinvader2(void);
void Sound_Fastinvader3(void);
void Sound_Fastinvader4(void);
#endif
