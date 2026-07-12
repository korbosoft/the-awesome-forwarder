#include <malloc.h>
#include "pngu.h"
#include "video.h"
#include "filelist.h"

static int imagewidth = 0;
static int imageheight = 0;
static bool customSplash = false;
u8 * GetImageData(u8 * splash)
{
    PNGUPROP imgProp;
    IMGCTX ctx;
	u8 * data = NULL;
	if (splash != NULL) {
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

	if (!png)
		return NULL;

	ctx = PNGU_SelectImageFromBuffer(png);
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

void Background_Show(float x, float y, float z, u8 * bgdata, u8 * icondata, float angle, float scaleX, float scaleY, u8 alpha)
{
    //16:9 to 4:3 correction if needed
    if(CONF_GetAspectRatio() != CONF_ASPECT_16_9)
    {
        scaleX *= 640.0f/720.0f;
        x += (imagewidth*scaleX - imagewidth)/2.0f;
    }

	/* Draw image */
	Menu_DrawImg(x, y, z, imagewidth, imageheight, bgdata, angle, scaleX, scaleY, alpha);
	if (!customSplash)
		Menu_DrawImg(x+295, y+197, z, 128, 48, icondata, angle, scaleX, scaleY, alpha);
    Menu_Render();
}

void fadein(u8 * bgdata, u8 * icondata)
{
	int i;

	/* fadein of image */
	for(i = 0; i < 255; i = i+10)
	{
		if(i>255) i = 255;
		Background_Show(0, 0, 0, bgdata, icondata, 0, 1, 1, i);
	}
}

void fadeout(u8 * bgdata, u8 * icondata)
{
	int i;

	/* fadeoout of image */
	for(i = 255; i > 1; i = i-7)
	{
		if(i < 0) i = 0;
		Background_Show(0, 0, 0, bgdata, icondata, 0, 1, 1, i);
	}
}
