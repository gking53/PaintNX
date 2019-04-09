#include "functions.h"
#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <turbojpeg.h>

extern Framebuffer fb;
extern u32 stride;
extern u32 point_id;
extern u32 touchCount;
extern int firstTime;
extern u32 *framebuffer;
extern s32 xold;
extern s32 yold;
extern u32 color;
u32 pos;
s32 r;
undoStruct* pHead = NULL;
u32 backgroundColor =0xFFFFFFFF; //start off with white background

u32 magnitude(s32 x, s32 y){
	return (x*x)+(y*y);
}

//a WIP function for taking a jpeg and putting it on the screen, had problems with resizing so ill leave it for now
int decompJpeg(const char* jpegName){
	u8* tempFrameBuffer = (u8*)calloc(FB_HEIGHT*FB_WIDTH,sizeof(u32));
	if(tempFrameBuffer == NULL) return -1;
	FILE* fp = fopen(jpegName,"r");
	if(fp==NULL){
		free(tempFrameBuffer);
		return -1;
	}
	framebuffer = (u32*)framebufferBegin(&fb,&stride);
	addUndo(framebuffer);
	tjhandle handle = tjInitDecompress();
	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char* jpegBuf = (unsigned char*)malloc(fsize);
	fread(jpegBuf,fsize,1,fp);
	fclose(fp);
	int height,width,jpegSubsamp,jpegColorspace;
	tjDecompressHeader3(handle,jpegBuf,fsize,&width,&height,&jpegSubsamp,&jpegColorspace);
	tjDecompress2(handle,jpegBuf,fsize,tempFrameBuffer,1280,0,720,TJPF_RGBA,TJFLAG_FASTDCT);
	memcpy(framebuffer,tempFrameBuffer,sizeof(u32)*FB_HEIGHT*FB_WIDTH);
	tjDestroy(handle);
	framebufferEnd(&fb);
	return 1;
}

//used for drawing pixels to the framebuffer
void updateScreen(s32 x, s32 y,s32 r){
	s32 i;
	s32 j;
	for(i=-r;i<=r;i++){
		for(j=-r;j<=r;j++){
			if(x+i<0 || y+j<0){
				continue;
			}
			if(x+i>=FB_WIDTH || y+j>=FB_HEIGHT){
				continue;
			}
			if(magnitude(i,j)<=(r*r)){
				pos = (y+j) * stride / sizeof(u32) + (x+i);
				framebuffer[pos] = color;
			}
		}
	}
}

//reset the screen
void clearScreen(void){
	framebuffer = (u32*)framebufferBegin(&fb, &stride);
	for(u32 y = 0; y < FB_HEIGHT; y++) {
        for(u32 x = 0; x < FB_WIDTH; x++) {
							pos = y * stride / sizeof(u32) + x;
							framebuffer[pos] = backgroundColor; //White
		}
	}
	framebufferEnd(&fb);
}

//changes the background color to the brush color
void background(void){
	backgroundColor = color;
}

//display (almost) all colors on screen
//uses algorithms for HSV colors and then converts them to RGB to be displayed (i understood HSV scrolling better than RGB :P)
void pickColor(void){
	u32* tempFrameBuffer = (u32*)calloc(FB_HEIGHT*FB_WIDTH,sizeof(u32));
	if(tempFrameBuffer==NULL) return;
	framebuffer = (u32*)framebufferBegin(&fb,&stride);
	memcpy(tempFrameBuffer,framebuffer,sizeof(u32)*FB_HEIGHT*FB_WIDTH);
	rgb pickRGB;
	hsv pickHSV;
	pickHSV.h = 0;
	for(u16 i = 0; i < FB_WIDTH; i++) {
		pickHSV.h = pickHSV.h + .27;
		if(pickHSV.h>360){
			pickHSV.h=0;
		}
		pickHSV.v = 1;
		pickHSV.s = 0;
        for(u16 j = 0; j < FB_HEIGHT; j++) {
			//pickHSV.v = pickHSV.v-(.00072*2);
			if(j>50){
				pickHSV.s = pickHSV.s+.005;
			}
			if(pickHSV.s >=1){
				pickHSV.s = 1;
				pickHSV.v = pickHSV.v-(.00072*3);
			}
			if(pickHSV.v<0){
				pickHSV.v=0;
			}
			pickRGB = hsv2rgb(pickHSV);
			u32 currentColor=0x0000FF00; //set up the alpha bits off the bat
			u8 tempColor;
			tempColor = pickRGB.b*255;
			currentColor = currentColor | tempColor;
			currentColor = currentColor << 8;
			tempColor = pickRGB.g*255;
			currentColor = currentColor | tempColor;
			currentColor = currentColor << 8;
			tempColor = pickRGB.r*255;
			currentColor = currentColor | tempColor;
      framebuffer[i + j * FB_WIDTH] = currentColor;
        }
    }
	framebufferEnd(&fb);
	hidScanInput();
	touchPosition touch;
	hidTouchRead(&touch,point_id);
	touchCount = hidTouchCount();
	while(touchCount<1){
		hidScanInput();
		hidTouchRead(&touch,point_id);
		touchCount = hidTouchCount();
		touchCount = hidTouchCount();
	}
	s32 x = touch.px;
	s32 y = touch.py;
	framebuffer = (u32*)framebufferBegin(&fb,&stride);
	pos = y * stride / sizeof(u32) + x;
	color=framebuffer[pos];
	while(touchCount>0){
		hidScanInput();
		hidTouchRead(&touch,point_id);
		touchCount = hidTouchCount();
		touchCount = hidTouchCount();
	}
	firstTime = 0;
	memcpy(framebuffer,tempFrameBuffer,sizeof(u32)*FB_HEIGHT*FB_WIDTH);
	free(tempFrameBuffer);
	framebufferEnd(&fb);
	return;
}

//used when the user wants to enter a certain brush radius
void enterRadius(void){
	SwkbdConfig kbd;
	char tmpoutstr[4] = {0};
	swkbdCreate(&kbd, 0);
	swkbdConfigMakePresetDefault(&kbd);
	kbd.arg.arg.type = SwkbdType_NumPad;
	kbd.arg.arg.stringLenMax = 3;
	swkbdConfigSetHeaderText(&kbd, "Enter brush radius");
	swkbdShow(&kbd, tmpoutstr, sizeof(tmpoutstr));
	if(tmpoutstr[0]!=0) r = atoi(tmpoutstr);
	swkbdClose(&kbd);
}

//sets the brush color to the background color
void eraser(void){
	color = backgroundColor;
}

//used to choose a color without displaying all colors
void tearDrop(void){
	hidScanInput();
	touchPosition touch;
	hidTouchRead(&touch,point_id);
	touchCount = hidTouchCount();
	while(touchCount<1){
		hidScanInput();
		hidTouchRead(&touch,point_id);
		touchCount = hidTouchCount();
		touchCount = hidTouchCount();
	}
	s32 x = touch.px;
	s32 y = touch.py;
	framebuffer = (u32*)framebufferBegin(&fb,&stride);
	pos = y * stride / sizeof(u32) + x;
	color=framebuffer[pos];
	while(touchCount>0){
		hidScanInput();
		hidTouchRead(&touch,point_id);
		touchCount = hidTouchCount();
		touchCount = hidTouchCount();
	}
	firstTime = 0;
	framebufferEnd(&fb);
}

//deals with the user input, called ever frame
int takeInput(void){
	// Scan all the inputs. This should be done once for each frame
	hidScanInput();
	// hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
	u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
	if (kDown & KEY_MINUS) clearScreen();
	if (kDown & KEY_PLUS) return 1; // break in order to return to hbmenu
	if (kDown & KEY_B) undo();
	if (kDown & KEY_A) pickColor();
	if (kDown & KEY_X) background();
	if (kDown & KEY_L) eraser();
	if (kDown & KEY_R) tearDrop();
	//if (kDown & KEY_Y) decompJpeg("/switch/cat.jpg");
	if((kDown & KEY_RIGHT)||(kDown & KEY_LEFT)) enterRadius();
	if (kDown & KEY_UP){
		if(r+1<100){
		r = r+1;
		}
	}
	if (kDown & KEY_DOWN){
		if(r-1>0){
			r = r - 1;
		}
	}
	return 0;
}

//called from main to deal with everyhitng needed for drawing
void draw(void){
	touchPosition touch;
	// Read the touch screen coordinates
	hidTouchRead(&touch,point_id);
	touchCount = hidTouchCount();
	s32 x = touch.px;
	s32 y = touch.py;
  framebuffer = (u32*)framebufferBegin(&fb, &stride);
	if(touchCount>=1){
		if(firstTime==0){
			addUndo(framebuffer);
			xold = x;
			yold = y;
			firstTime=1;
		}
		// Bresenham's line algorithm, thanks to 3DS_Paint norips for this implementation
		int dx = abs(x-xold);
		int sx = xold<x ? 1 : -1;
		int dy = abs(y-yold);
		int sy = yold<y ? 1 : -1;
		int err = (dx>dy ? dx : -dy)/2;
		int e2;

		while(1){
			updateScreen(xold,yold,r);
			if (xold==x && yold==y) break;
			e2 = err;
			if (e2 >-dx) { err -= dy; xold += sx; }
			if (e2 < dy) { err += dx; yold += sy; }
		}
	}
	if(touchCount==0){
		firstTime = 0;
	}
	xold = x;
	yold = y;
	framebufferEnd(&fb);
}

//functions for the undo button, linked list (stack) as long as ram will allow you to allocate theoretically
void undo(void){
	if(pHead == NULL) return;
	undoStruct* tempStruct = pHead->nextStruct;
	framebuffer  = (u32*)framebufferBegin(&fb,&stride);
	memcpy(framebuffer,pHead->oldFrameBuff,sizeof(u32)*FB_HEIGHT*FB_WIDTH);
	free(pHead->oldFrameBuff);
	free(pHead);
	pHead = tempStruct;
	framebufferEnd(&fb);
}

void addUndo(u32* inputBuffer){
	undoStruct* tempStruct = NULL;
	tempStruct = (undoStruct*)calloc(1,sizeof(undoStruct));
	if(tempStruct == NULL) return;
	tempStruct->oldFrameBuff = (u32*)calloc(1,sizeof(u32)*FB_WIDTH*FB_HEIGHT);
	if(tempStruct->oldFrameBuff == NULL){
		free(tempStruct);
		return;
	}
	memcpy(tempStruct->oldFrameBuff,inputBuffer,sizeof(u32)*FB_HEIGHT*FB_WIDTH);
	tempStruct->nextStruct = pHead;
	pHead = tempStruct;
}

//many functions used for the color display / choosing screen used for scrolling the colors across the x and y plane, taken from a stack overflow thread, no source on the thread either :( user said he
//has been using it for years and forgot where it came from, works beautifully though!
float redP(int H, float L){
	if(H>=0 && H<=60){
		return findC(L);
	}
	else if(H>=60 && H<=120){
		return findX(findC(L),H);
	}
	else if(H>=120 && H<=180){
		return 0;
	}
	else if(H>=180 && H<=240){
		return 0;
	}
	else if(H>=240 && H<=300){
		return findX(findC(L),H);
	}
	else if(H>=300 && H<=360){
		return findC(L);
	}
	return 0;
}
float blueP(int H, float L){
	if(H>=0 && H<=60){
		return findX(findC(L),H);
	}
	else if(H>=60 && H<=120){
		return findC(L);
	}
	else if(H>=120 && H<=180){
		return findC(L);
	}
	else if(H>=180 && H<=240){
		return findX(findC(L),H);
	}
	else if(H>=240 && H<=300){
		return 0;
	}
	else if(H>=300 && H<=360){
		return 0;
	}
	return 0;
}

float greenP(int H, float L){
	if(H>=0 && H<=60){
		return 0;
	}
	else if(H>=60 && H<=120){
		return 0;
	}
	else if(H>=120 && H<=180){
		return findX(findC(L),H);
	}
	else if(H>=180 && H<=240){
		return findC(L);
	}
	else if(H>=240 && H<=300){
		return findC(L);
	}
	else if(H>=300 && H<=360){
		return findX(findC(L),H);
	}
	return 0;
}

float findC(float L){
	return (1-fabs(2*L-1));
}

float findX(float C, int H){
	return (C*(1-abs(fmod((H/60),2)-1)));
}

float findM(float L, float C){
	return (L-C/2);
}

hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}


rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;
}

//needed for software keyboard
void userAppInit(void)
{
   void *addr = NULL;
   svcSetHeapSize(&addr, 0x4000000);
}
