#ifndef _DEVICEMOUNTER_H_
#define _DEVICEMOUNTER_H_

enum
{
	SD = 0,
	USB1,
	USB2,
	USB3,
	USB4,
	MAXDEVICES
};

static const char DeviceName[MAXDEVICES][6] =
{
	"sd",
	"usb1",
	"usb2",
	"usb3",
	"usb4"
};

int USBDevice_Init();
void USBDevice_deInit();
int SDCard_Init();
void SDCard_deInit();

#endif
