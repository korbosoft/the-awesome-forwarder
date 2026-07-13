#include "config.h"

#ifdef SPLASH

#include <malloc.h>
#include <gccore.h>
#include "pngu.h"
#include "video.h"

#include "background_png.h"
#include "icon_fallback_png.h"

static int imagewidth = 0;
static int imageheight = 0;
static bool customSplash = false;
u8 * GetImageData(u8 * splash)
{
    PNGUPROP imgProp;
    IMGCTX ctx;
	u8 * data = NULL;
	if (splash) {
		ctx = PNGU_SelectImageFromBuffer(splash);
		customSplash = true;
	} else {
		ctx = PNGU_SelectImageFromBuffer(background_png);
	}

	if (!ctx)
		return NULL;

	if (PNGU_GetImageProperties(ctx, &imgProp) != PNGU_OK)
	{
		PNGU_ReleaseImageContext(ctx);
		return NULL;
	}

    imagewidth = imgProp.imgWidth;
    imageheight = imgProp.imgHeight;

    int len = ((((imgProp.imgWidth+3)>>2)*((imgProp.imgHeight+3)>>2)*32*2) + 31) & ~31;

    data = PNGU_DecodeTo4x4RGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, (int *)&imgProp.imgWidth, (int *)&imgProp.imgHeight);

	DCFlushRange(data, len);

	PNGU_ReleaseImageContext(ctx);

    return data;
}

u8 * GetIconData(u8 *png)
{
	PNGUPROP imgProp;
	IMGCTX ctx;
	u8 * data = NULL;

	ctx = PNGU_SelectImageFromBuffer(png ?: icon_fallback_png);
	if (!ctx)
		return NULL;

	if (PNGU_GetImageProperties(ctx, &imgProp) != PNGU_OK)
	{
		PNGU_ReleaseImageContext(ctx);
		return NULL;
	}

	int len = ((((imgProp.imgWidth + 3) >> 2) * ((imgProp.imgHeight + 3) >> 2) * 32 * 2) + 31) & ~31;

	data = PNGU_DecodeTo4x4RGBA8(ctx, imgProp.imgWidth, imgProp.imgHeight, (int *)&imgProp.imgWidth, (int *)&imgProp.imgHeight);

	DCFlushRange(data, len);

	PNGU_ReleaseImageContext(ctx);

	return data;
}

void Background_Show(float x, float y, float z, u8 * bgdata, u8 * icondata, u8 alpha)
{
	Menu_DrawImg(x, y, z, imagewidth, imageheight, bgdata);

	if (!customSplash)
		Menu_DrawImg(x + 255.0f, y + 197.0f, z, 128, 48, icondata);

	Menu_DrawQuad(0, 0, 0, alpha);
	Menu_Render();
}

void fadein(u8 * bgdata, u8 * icondata)
{
	int i;

	/* fadein of image */
	for(i = 255; i > 0; i -= 10)
	{
		if(i>255) i = 255;
		Background_Show(0, 0, 0, bgdata, icondata, i);
	}
}

void fadeout(u8 * bgdata, u8 * icondata)
{
	int i;

	/* fadeout of image */
	for(i = 0; i < 255; i += 7)
	{
		if(i < 0) i = 0;
		Background_Show(0, 0, 0, bgdata, icondata, i);
	}
}

#endif
