/****************************************************************************
 * Copyright 2009 The Lemon Man and thanks to luccax, Wiipower, Aurelio and crediar
 * Copyright 2010 Dimok
 * Copyright 2011 oggzee
 * Copyright 2011-2012 FIX94
 * Copyright 2026 Korbosoft
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 ***************************************************************************/
#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ogc/machine/processor.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "splash.h"
#include "devicemounter.h"
#include "video.h"
#include "config.h"

#define EXECUTE_ADDR	((u8 *)0x92000000)
#define BOOTER_ADDR		((u8 *)0x93000000)
#define ARGS_ADDR		((u8 *)0x93200000)

#define Priiloader_CFG1	((vu32*)0x8132FFFB)
#define Priiloader_CFG2	((vu32*)0x817FEFF0)

#define MAX_CMDLINE 4096
#define MAX_ARGV    1000
struct __argv args;
char *a_argv[MAX_ARGV];
char *meta_buf = NULL;
u8 *icon_buf = NULL;
u8 *splash_buf = NULL;

// extern void __exception_setreload(int t);

typedef void (*entrypoint) (void);
#include "app_booter_bin.h"

#define PATH_COUNT 2

static const char* const PATHS[PATH_COUNT] = {
	"/apps/korbodonut/boot.dol",
	"/apps/korbodonut/boot.elf"
};

void SystemMenu()
{
	// Priiloader code disabled to respect user settings

	/* If priiloader is installed say return to system menu */
	// *Priiloader_CFG1 = 0x50756E65;
	// DCFlushRange((void*)Priiloader_CFG1, 4);
	// *Priiloader_CFG2 = 0x50756E65;
	// DCFlushRange((void*)Priiloader_CFG2, 4);
	/* Use Return to Menu to exit */
	SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}

static bool IsDollZ(u8 *buf)
{
	u8 cmp1[] = {0x3C};
	return memcmp(&buf[0x100], cmp1, sizeof(cmp1)) == 0;
}

static bool IsSpecialELF(u8 *buf)
{
	u32 cmp1[] = {0x7F454C46};
	u8 cmp2[] = {0x00};
	return memcmp(buf, cmp1, sizeof(cmp1)) == 0 && memcmp(&buf[0x24], cmp2, sizeof(cmp2)) == 0;
}

void arg_init()
{
	memset(&args, 0, sizeof(args));
	memset(a_argv, 0, sizeof(a_argv));
	args.argvMagic = ARGV_MAGIC;
	args.length = 1; // double \0\0
	args.argc = 0;
	//! Put the argument into mem2 too, to avoid overwriting it
	args.commandLine = (char *) ARGS_ADDR + sizeof(args);
	args.argv = a_argv;
	args.endARGV = a_argv;
}

char* strcopy(char *dest, const char *src, int size)
{
	if (size <= 0) return dest;
	strncpy(dest, src, size);
	dest[size-1] = 0;
	return dest;
}

int arg_addl(char *arg, int len)
{
	if (args.argc >= MAX_ARGV)
		return -1;
	if (args.length + len + 1 > MAX_CMDLINE)
		return -1;
	strcopy(args.commandLine + args.length - 1, arg, len + 1);
	args.length += len + 1; // 0 term.
	args.commandLine[args.length - 1] = 0; // double \0\0
	args.argc++;
	args.endARGV = args.argv + args.argc;
	return 0;
}

int arg_add(char *arg)
{
	return arg_addl(arg, strlen(arg));
}

void load_meta(const char *exe_path)
{
	char meta_path[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32);
	memset(meta_path, 0, ISFS_MAXPATH);
	const char *p;

	p = strrchr(exe_path, '/');
	snprintf(meta_path, sizeof(meta_path), "%.*smeta.xml", p ? p-exe_path+1 : 0, exe_path);

	s32 fd = ISFS_Open(meta_path, ISFS_OPEN_READ);
	if(fd >= 0)
	{
		s32 filesize = ISFS_Seek(fd, 0, SEEK_END);
		ISFS_Seek(fd, 0, SEEK_SET);
		if(filesize)
		{
			meta_buf = (char*)memalign(32, filesize + 1);
			memset(meta_buf, 0, filesize + 1);
			ISFS_Read(fd, meta_buf, filesize);
		}
		ISFS_Close(fd);
	}
	else
	{
		FILE *exeFile = fopen(meta_path ,"rb");
		if(exeFile)
		{
			fseek(exeFile, 0, SEEK_END);
			u32 exeSize = ftell(exeFile);
			rewind(exeFile);
			if(exeSize)
			{
				meta_buf = (char*)memalign(32, exeSize + 1);
				memset(meta_buf, 0, exeSize + 1);
				fread(meta_buf, 1, exeSize, exeFile);
			}
			fclose(exeFile);
		}
	}
}

#ifdef SPLASH
void load_icon(const char *exe_path)
{
	char icon_path[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32);
	memset(icon_path, 0, ISFS_MAXPATH);
	const char *p;

	p = strrchr(exe_path, '/');
	snprintf(icon_path, sizeof(icon_path), "%.*sicon.png", p ? p-exe_path+1 : 0, exe_path);

	s32 fd = ISFS_Open(icon_path, ISFS_OPEN_READ);
	if(fd >= 0)
	{
		s32 filesize = ISFS_Seek(fd, 0, SEEK_END);
		ISFS_Seek(fd, 0, SEEK_SET);
		if(filesize)
		{
			icon_buf = (u8*)memalign(32, filesize + 1);
			memset(icon_buf, 0, filesize + 1);
			ISFS_Read(fd, icon_buf, filesize);
		}
		ISFS_Close(fd);
	}
	else
	{
		FILE *iconFile = fopen(icon_path ,"rb");
		if(iconFile)
		{
			fseek(iconFile, 0, SEEK_END);
			u32 iconSize = ftell(iconFile);
			rewind(iconFile);
			if(iconSize)
			{
				icon_buf = (u8*)memalign(32, iconSize + 1);
				memset(icon_buf, 0, iconSize + 1);
				fread(icon_buf, 1, iconSize, iconFile);
			}
			fclose(iconFile);
		}
	}
}

void load_splash(const char *exe_path)
{
	char splash_path[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32);
	memset(splash_path, 0, ISFS_MAXPATH);
	const char *p;

	p = strrchr(exe_path, '/');
	snprintf(splash_path, sizeof(splash_path), "%.*ssplash.png", p ? p-exe_path+1 : 0, exe_path);

	s32 fd = ISFS_Open(splash_path, ISFS_OPEN_READ);
	if(fd >= 0)
	{
		s32 filesize = ISFS_Seek(fd, 0, SEEK_END);
		ISFS_Seek(fd, 0, SEEK_SET);
		if(filesize)
		{
			splash_buf = (u8*)memalign(32, filesize + 1);
			memset(splash_buf, 0, filesize + 1);
			ISFS_Read(fd, splash_buf, filesize);
		}
		ISFS_Close(fd);
	}
	else
	{
		FILE *splashFile = fopen(splash_path ,"rb");
		if(splashFile)
		{
			fseek(splashFile, 0, SEEK_END);
			u32 splashSize = ftell(splashFile);
			rewind(splashFile);
			if(splashSize)
			{
				splash_buf = (u8*)memalign(32, splashSize + 1);
				memset(splash_buf, 0, splashSize + 1);
				fread(splash_buf, 1, splashSize, splashFile);
			}
			fclose(splashFile);
		}
	}
}
#endif

void strip_comments(char *buf)
{
	char *p = buf; // start of comment
	char *e; // end of comment
	int len;
	while (p && *p)
	{
		p = strstr(p, "<!--");
		if (!p)
			break;
		e = strstr(p, "-->");
		if (!e)
		{
			*p = 0; // terminate
			break;
		}
		e += 3;
		len = strlen(e);
		memmove(p, e, len + 1); // +1 for 0 termination
	}
}

void parse_meta()
{
	if (meta_buf == NULL)
		return;

	strip_comments(meta_buf);

	if (strstr(meta_buf, "<app") && strstr(meta_buf, "</app>"))
	{
		char *p = strstr(meta_buf, "<arguments>");
		char *end = strstr(meta_buf, "</arguments>");

		if (p && end)
		{
			bool tracking = true;
			while (tracking && p < end)
			{
				p = strstr(p, "<arg>");
				if (!p)
				{
					tracking = false;
				}
				else
				{
					p += 5; // strlen("<arg>")
					char *e = strstr(p, "</arg>");
					if (!e)
					{
						tracking = false;
					}
					else
					{
						arg_addl(p, e - p);
						p = e + 6;
					}
				}
			}
		}
	}

	if (meta_buf)
	{
		free(meta_buf);
		meta_buf = NULL;
	}
}

static void DeInitDevices()
{
	SDCard_deInit();
	USBDevice_deInit();
	ISFS_Deinitialize();
}

int main(int argc, char *argv[])
{
	u32 cookie;
	entrypoint exeEntryPoint;

#ifdef SPLASH
	/* Initialize video layout structures */
	InitVideo();
#endif

	char full_path[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32);
	memset(full_path, 0, ISFS_MAXPATH);
	bool FileFound = false;

	/* Try NAND (neek2o) first */
	ISFS_Initialize();
	for(u8 i = 0; i < PATH_COUNT; i++)
	{
		strcpy(full_path, PATHS[i]);
		s32 fd = ISFS_Open(full_path, ISFS_OPEN_READ);
		if(fd >= 0)
		{
			s32 filesize = ISFS_Seek(fd, 0, SEEK_END);
			ISFS_Seek(fd, 0, SEEK_SET);
			if(filesize)
			{
				ISFS_Read(fd, EXECUTE_ADDR, filesize);
				FileFound = true;
			}
			ISFS_Close(fd);
			break;
		}
	}

	/* Try External Storage Devices (SD then USB) if not on NAND */
	if(!FileFound)
	{
		bool DeviceFound = false;
		for(u8 dev = SD; dev < MAXDEVICES && !DeviceFound; ++dev)
		{
			if(dev == SD)
				SDCard_Init();
			else if(dev == USB1)
				USBDevice_Init();

			for(u8 i = 0; i < PATH_COUNT; i++)
			{
				sprintf(full_path, "%s:%s", DeviceName[dev], PATHS[i]);
				FILE *exeFile = fopen(full_path, "rb");
				if(exeFile)
				{
					fseek(exeFile, 0, SEEK_END);
					u32 exeSize = ftell(exeFile);
					rewind(exeFile);
					if(exeSize)
					{
						fread(EXECUTE_ADDR, 1, exeSize, exeFile);
						FileFound = true;
						DeviceFound = true; // Signals outer loop to stop completely
					}
					fclose(exeFile);
					break; // Breaks out of the path choice inner loop
				}
			}
		}
	}

#ifdef SPLASH
	load_splash(full_path);
	u8 * bgdata = GetImageData(splash_buf);
	u8 * icondata = NULL;
	if (bgdata) {
		load_icon(full_path);
		icondata = GetIconData(icon_buf);
	}
#ifdef SPLASH_FADE_IN
	fadein(bgdata, icondata);
#else
	Background_Show(0, 0, 0, bgdata, icondata);
#endif

#ifdef SPLASH_FADE_OUT
	fadeout(bgdata, icondata);
#endif

	StopGX();
	free(bgdata);
#endif

	DeInitDevices();

	if(!FileFound)
	{
		SystemMenu();
		return 0;
	}
	else
	{
		if(!IsDollZ(EXECUTE_ADDR) && !IsSpecialELF(EXECUTE_ADDR))
		{
			arg_init();
			arg_add(full_path); // argv[0] = full_path
			load_meta(full_path);
			parse_meta();
		}
	}

	memcpy(BOOTER_ADDR, app_booter_bin, app_booter_bin_size);
	DCFlushRange(BOOTER_ADDR, app_booter_bin_size);

	memcpy(ARGS_ADDR, &args, sizeof(args));
	DCFlushRange(ARGS_ADDR, sizeof(args) + args.length);

	exeEntryPoint = (entrypoint)BOOTER_ADDR;

	/* Clean up hardware states and boot executable target */
	SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
	_CPU_ISR_Disable(cookie);
	exeEntryPoint();
	_CPU_ISR_Restore(cookie);

	SystemMenu();
	return 0;
}
