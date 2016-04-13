// Don't edit this file!  It's software-generated.
// See convert.py script instead.

#define PALETTE1  0
#define PALETTE4  1
#define PALETTE8  2
#define TRUECOLOR 3

#define NUM_LEDS 14

// test.gif ----------------------------------------------------------------

const uint8_t PROGMEM palette00[][3] = {
  { 255,   0,   0 },
  {   0,  38, 255 },
  {   4, 255,   0 },
  { 255, 255, 255 } };

const uint8_t PROGMEM pixels00[] = {
  0X33, 0X33, 0X31, 0X11, 0X33, 0X33, 0X33, 0X33,
  0X33, 0X31, 0X11, 0X33, 0X33, 0X33, 0X33, 0X33,
  0X33, 0X13, 0X33, 0X33, 0X33, 0X33, 0X33, 0X33,
  0X13, 0X33, 0X33, 0X33, 0X33, 0X33, 0X33, 0X13,
  0X33, 0X33, 0X33, 0X33, 0X33, 0X33, 0X13, 0X33,
  0X33, 0X33, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X33, 0X33, 0X33, 0X23, 0X33, 0X33, 0X33, 0X33,
  0X33, 0X33, 0X23, 0X33, 0X33, 0X33, 0X33, 0X33,
  0X33, 0X23, 0X33, 0X33, 0X33, 0X33, 0X33, 0X33,
  0X23, 0X33, 0X33, 0X33, 0X33, 0X33, 0X32, 0X22,
  0X33, 0X33, 0X33, 0X33, 0X33, 0X32, 0X22, 0X33,
  0X33, 0X33 };

typedef struct {
  uint8_t        type;    // PALETTE[1,4,8] or TRUECOLOR
  line_t         lines;   // Length of image (in scanlines)
  const uint8_t *palette; // -> PROGMEM color table (NULL if truecolor)
  const uint8_t *pixels;  // -> Pixel data in PROGMEM
} image;

const image PROGMEM images[] = {
  { PALETTE4 ,   14, (const uint8_t *)palette00, pixels00 }
};

#define NUM_IMAGES (sizeof(images) / sizeof(images[0]))
