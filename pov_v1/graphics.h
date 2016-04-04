// Don't edit this file!  It's software-generated.
// See convert.py script instead.

#define PALETTE1  0
#define PALETTE4  1
#define PALETTE8  2
#define TRUECOLOR 3

#define NUM_LEDS 14

// blackfive.gif -----------------------------------------------------------

const uint8_t PROGMEM palette00[][3] = {
  {   0,   0,   0 },
  { 255,   0,   0 },
  { 255, 255, 255 } };

const uint8_t PROGMEM pixels00[] = {
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X11,
  0X10, 0X01, 0X00, 0X00, 0X00, 0X02, 0X00, 0X20,
  0X02, 0X00, 0X00, 0X00, 0X02, 0X00, 0X20, 0X02,
  0X00, 0X00, 0X00, 0X02, 0X00, 0X20, 0X02, 0X00,
  0X00, 0X00, 0X02, 0X00, 0X20, 0X02, 0X00, 0X00,
  0X00, 0X02, 0X00, 0X20, 0X02, 0X00, 0X00, 0X00,
  0X01, 0X00, 0X11, 0X11, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00 };

// lines.gif ---------------------------------------------------------------

const uint8_t PROGMEM palette01[][3] = {
  { 255,   0,   0 },
  {   0, 255,   7 },
  {   0, 247, 255 } };

const uint8_t PROGMEM pixels01[] = {
  0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22,
  0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22,
  0X22, 0X22, 0X22, 0X22, 0X22, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X11, 0X11, 0X11, 0X11, 0X11,
  0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11,
  0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11, 0X11,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X22, 0X22,
  0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22,
  0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22,
  0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22, 0X22,
  0X22, 0X22 };

// twoStrip.gif ------------------------------------------------------------

const uint8_t PROGMEM palette02[][3] = {
  {   0, 212, 255 },
  {   7, 255,   0 },
  { 255,   0,   0 } };

const uint8_t PROGMEM pixels02[] = {
  0X02, 0X22, 0X22, 0X22, 0X22, 0X22, 0X21, 0X20,
  0X22, 0X22, 0X22, 0X22, 0X22, 0X12, 0X22, 0X02,
  0X22, 0X22, 0X22, 0X21, 0X22, 0X22, 0X20, 0X22,
  0X22, 0X22, 0X12, 0X22, 0X22, 0X22, 0X02, 0X22,
  0X21, 0X22, 0X22, 0X22, 0X22, 0X20, 0X22, 0X12,
  0X22, 0X22, 0X22, 0X22, 0X22, 0X01, 0X22, 0X22,
  0X22, 0X22, 0X22, 0X22, 0X10, 0X22, 0X22, 0X22,
  0X22, 0X22, 0X21, 0X22, 0X02, 0X22, 0X22, 0X22,
  0X22, 0X12, 0X22, 0X20, 0X22, 0X22, 0X22, 0X21,
  0X22, 0X22, 0X22, 0X02, 0X22, 0X22, 0X12, 0X22,
  0X22, 0X22, 0X20, 0X22, 0X21, 0X22, 0X22, 0X22,
  0X22, 0X22, 0X02, 0X12, 0X22, 0X22, 0X22, 0X22,
  0X22, 0X20 };

typedef struct {
  uint8_t        type;    // PALETTE[1,4,8] or TRUECOLOR
  line_t         lines;   // Length of image (in scanlines)
  const uint8_t *palette; // -> PROGMEM color table (NULL if truecolor)
  const uint8_t *pixels;  // -> Pixel data in PROGMEM
} image;

const image PROGMEM images[] = {
  { PALETTE4 ,   14, (const uint8_t *)palette00, pixels00 },
  { PALETTE4 ,   14, (const uint8_t *)palette01, pixels01 },
  { PALETTE4 ,   14, (const uint8_t *)palette02, pixels02 }
};

#define NUM_IMAGES (sizeof(images) / sizeof(images[0]))
