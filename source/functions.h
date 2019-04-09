#define FUNCTIONS_H
#define FB_WIDTH  1280
#define FB_HEIGHT 720


typedef unsigned int u32;
typedef int s32;
typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

struct undoStruct{
  undoStruct* nextStruct;
  u32* oldFrameBuff;
};

//needed for software keyboard input
void userAppInit(void);

//for pick color
hsv rgb2hsv(rgb in);
rgb hsv2rgb(hsv in);
float redP(int H, float L);
float blueP(int H, float L);
float greenP(int H, float L);
float findX(float C, int H);
float findC(float L);
float findM(float L, float C);

//for normal drawing and user functions
u32 magnitude(int x, int y); //calculate magnitude
void updateScreen(int x, int y,int r); //update a point on the screen / draw pixels on screen
void clearScreen(void); //clear the screen to the background color
void pickColor(void); //pick a new color and display all colors
int decompJpeg(const char* jpegName); //WIP function to display a saved jpeg to draw on
int takeInput(void); //all button input handled here
void draw(void); //function to deal with user touches and drawing
void undo(void); //undo the last drawing action
void addUndo(u32* inputBuffer); //called after a touch happens to add the last framebuffer to the queue
void enterRadius(void); //user input radius instead of buttons
void eraser(void); //"erase" the screen by setting the brush to the background color
void tearDrop(void); //take a color from the screen and set it to the brush
