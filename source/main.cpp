#include <switch.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include "functions.h"

Framebuffer fb;
u32 stride;
u32 *framebuffer;
u32 *oldFrameBuf[10];
u32 point_id = 0;
u32 touchCount;
int firstTime;
s32 xold;
s32 yold;
u32 color = 0xFF000000;

int main(int argc, char **argv)
{
	userAppInit();
	firstTime = 0;
	NWindow* win = nwindowGetDefault();
	framebufferCreate(&fb, win, FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
	framebufferMakeLinear(&fb);

	hidInitialize();
    // set the background to all white to start
	clearScreen();

    while (appletMainLoop())
    {
		if(takeInput()==1) break;;
		draw();
    }
    // Exit services
    framebufferClose(&fb);
		hidExit();
    return 0;
}
