// Don't edit this file!  It's software-generated.
// See convert.py script instead.

#define PALETTE1  0
#define PALETTE4  1
#define PALETTE8  2
#define TRUECOLOR 3

#define NUM_LEDS 16

// lines16.gif -------------------------------------------------------------

const uint8_t PROGMEM palette00[][3] = {
  {   0,   1, 255 },
  { 255,   0,   0 },
  {   0, 255,   0 } };

const uint8_t PROGMEM pixels00[] = {
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11,
  0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11,
  0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22,
  0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22,
  0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11,
  0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00 };

typedef struct {
  uint8_t        type;    // PALETTE[1,4,8] or TRUECOLOR
  line_t         lines;   // Length of image (in scanlines)
  const uint8_t *palette; // -> PROGMEM color table (NULL if truecolor)
  const uint8_t *pixels;  // -> Pixel data in PROGMEM
} image;

const image PROGMEM images[] = {
  { PALETTE4 ,   16, (const uint8_t *)palette00, pixels00 }
};

#define NUM_IMAGES (sizeof(images) / sizeof(images[0]))