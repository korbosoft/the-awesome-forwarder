#ifndef BACKGROUND_IMAGE_H_
#define BACKGROUND_IMAGE_H_

#ifdef __cplusplus
extern "C"
{
#endif

u8 * GetImageData(u8 * splash);
u8 * GetIconData(u8 *png);
void Background_Show(float x, float y, float z, u8 * bgdata, u8 * icon, float angle, float scaleX, float scaleY, u8 alpha);
void fadein(u8 * bgdata, u8 * icondata);
void fadeout(u8 * bgdata, u8 * icondata);

#ifdef __cplusplus
}
#endif

#endif
