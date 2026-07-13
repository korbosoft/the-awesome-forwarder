/****************************************************************************
 * Tantric 2009
 * Korbosoft 2026
 *
 * video.cpp
 * Video routines
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define DEFAULT_FIFO_SIZE 256 * 1024
static unsigned int *xfb[2] = {NULL, NULL}; // Double buffered
static int whichfb = 0; // Switch
static GXRModeObj *vmode; // Menu video mode
static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN(32);
static Mtx GXmodelView2D;
f32 scaleX;

/****************************************************************************
 * InitVideo
 *
 * This function MUST be called at startup.
 * - also sets up menu video mode
 ***************************************************************************/

void InitVideo(void)
{
	Mtx44 p;
	f32 yscale;
	u32 xfbHeight;

	VIDEO_Init();
	vmode = VIDEO_GetPreferredMode(NULL); // get default video mode

	VIDEO_Configure(vmode);

	// Allocate the video buffers
	xfb[0] = (u32 *) MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	xfb[1] = (u32 *) MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));

	// Clear framebuffers etc.
	VIDEO_ClearFrameBuffer(vmode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer(vmode, xfb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer(xfb[0]);

	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if (vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
	else
		while (VIDEO_GetNextField())
			VIDEO_WaitVSync();

	/*** Clear out FIFO area ***/
	memset(&gp_fifo, 0, DEFAULT_FIFO_SIZE);

	/*** Initialise GX ***/
	GX_Init(&gp_fifo, DEFAULT_FIFO_SIZE);
	GX_SetCopyClear((GXColor){0, 0, 0, 255}, 0x00ffffff);

	GX_SetDispCopyGamma(GX_GM_1_0);
	GX_SetCullMode(GX_CULL_NONE);

	yscale = GX_GetYScaleFactor(vmode->efbHeight,vmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopySrc(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopyDst(vmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(vmode->aa,vmode->sample_pattern,GX_TRUE,vmode->vfilter);
	GX_SetFieldMode(vmode->field_rendering,((vmode->viHeight==2*vmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (vmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	// setup the vertex descriptor
	// tells the flipper to expect direct data
	GX_ClearVtxDesc();
	GX_InvVtxCache();
	GX_InvalidateTexAll();

	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 0);
	GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	guMtxIdentity(GXmodelView2D);
	guMtxTransApply(GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -200.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);

	guOrtho(p,0,vmode->efbHeight-1,0,vmode->fbWidth-1,0,300);
	GX_LoadProjectionMtx(p, GX_ORTHOGRAPHIC);

	GX_SetViewport(0,0,vmode->fbWidth,vmode->efbHeight,0,1);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
}

/****************************************************************************
 * StopGX
 *
 * Stops GX (when exiting)
 ***************************************************************************/
void StopGX(void)
{
	GX_AbortFrame();
	GX_Flush();

	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
}

/****************************************************************************
 * Menu_Render
 *
 * Renders everything current sent to GX, and flushes video
 ***************************************************************************/
void Menu_Render(void)
{
	GX_DrawDone();

	whichfb ^= 1; // flip framebuffer
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
	GX_CopyDisp(xfb[whichfb],GX_TRUE);
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
	VIDEO_WaitVSync();
}

/****************************************************************************
 * Menu_DrawImg
 *
 * Draws the specified image on screen using GX
 ***************************************************************************/
void Menu_DrawImg(f32 xpos, f32 ypos, f32 zpos, u16 width, u16 height, u8 data[])
{
	if(data == NULL)
		return;

	GXTexObj texObj;
	GX_InitTexObj(&texObj, data, width, height, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);
	GX_InvalidateTexAll();

	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	Mtx m, mv;
	guMtxIdentity(m);

	guMtxTransApply(m, m, 0.0f, 0.0f, zpos);
	guMtxConcat(GXmodelView2D, m, mv);
	GX_LoadPosMtxImm(mv, GX_PNMTX0);

	s16 xLeft   = (s16)(xpos + 0.5f);
	s16 xRight  = (s16)(xpos + width + 0.5f);
	s16 yTop    = (s16)(ypos + 0.5f);
	s16 yBottom = (s16)(ypos + height + 0.5f);

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	GX_Position2s16(xLeft, yTop);
	GX_Color4u8(0xFF, 0xFF, 0xFF, 0xFF);
	GX_TexCoord2u8(0, 0);

	GX_Position2s16(xRight, yTop);
	GX_Color4u8(0xFF, 0xFF, 0xFF, 0xFF);
	GX_TexCoord2u8(1, 0);

	GX_Position2s16(xRight, yBottom);
	GX_Color4u8(0xFF, 0xFF, 0xFF, 0xFF);
	GX_TexCoord2u8(1, 1);

	GX_Position2s16(xLeft, yBottom);
	GX_Color4u8(0xFF, 0xFF, 0xFF, 0xFF);
	GX_TexCoord2u8(0, 1);
	GX_End();

	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
}

void Menu_DrawQuad(u8 r, u8 g, u8 b, u8 alpha)
{
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);

	// Load the clean base matrix layout structure directly
	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

	// Draw directly mapping the full screen size limits
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	GX_Position2s16(0, 0);
	GX_Color4u8(r, g, b, alpha);

	GX_Position2s16(vmode->fbWidth, 0);
	GX_Color4u8(r, g, b, alpha);

	GX_Position2s16(vmode->fbWidth,vmode->efbHeight);
	GX_Color4u8(r, g, b, alpha);

	GX_Position2s16(0, vmode->efbHeight);
	GX_Color4u8(r, g, b, alpha);
	GX_End();
}
