/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * video.h
 * Video routines
 ***************************************************************************/
#include "config.h"

#ifdef SPLASH

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <ogcsys.h>

void InitVideo(void);
void StopGX(void);
void ResetVideo_Menu(void);
void Menu_Render(void);
void Menu_DrawImg(f32 xpos, f32 ypos, f32 zpos, u16 width, u16 height, u8 data[]);
void Menu_DrawQuad(u8 r, u8 g, u8 b, u8 alpha);

#endif
#endif
