#include "config.h"

#ifdef SPLASH
#ifndef _SPLASH_H_
#define _SPLASH_H_

u8 * GetImageData(u8 * splash);
u8 * GetIconData(u8 *png);
void Background_Show(float x, float y, float z, u8 * bgdata, u8 * icondata);
void fadein(u8 * bgdata, u8 * icondata);
void fadeout(u8 * bgdata, u8 * icondata);

#endif
#endif
