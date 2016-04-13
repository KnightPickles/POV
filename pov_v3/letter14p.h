// Don't edit this file!  It's software-generated.
// See convert.py script instead.

#define PALETTE1  0
#define PALETTE4  1
#define PALETTE8  2
#define TRUECOLOR 3

#define NUM_LEDS 14

// letter14polar.gif -------------------------------------------------------

const uint8_t PROGMEM palette00[][3] = {
  {   0,   0,   0 },
  {   0,   0,   0 },
  {   0,   0,   0 },
  {   0,   0,   0 },
  {   0,   0,   1 },
  {   0,   0,   1 },
  {   0,   1,   1 },
  {   0,   1,   2 },
  {   0,   1,   3 },
  {   0,   2,   4 },
  {   0,   2,   5 },
  {   0,   3,   6 },
  {   0,   4,  10 },
  {   0,   5,  10 },
  {   0,   7,  15 },
  {   0,  12,  26 },
  {   0,  12,  27 },
  {   0,  17,  37 },
  {   0,  18,  38 },
  {   0,  18,  39 },
  {   0,  19,  41 },
  {   0,  22,  48 },
  {   0,  24,  53 },
  {   0,  25,  55 },
  {   0,  29,  62 },
  {   0,  30,  64 },
  {   0,  30,  66 },
  {   0,  45,  97 },
  {   0,  45,  98 },
  {   0,  47, 100 },
  {   0,  52, 112 },
  {   0,  61, 131 },
  {   0,  65, 141 },
  {   0,  70, 151 },
  {   0,  80, 173 },
  {   0,  91, 198 },
  {   0,  92, 200 },
  {   0,  94, 202 },
  {   0,  98, 212 },
  {   0, 101, 219 },
  {   0, 119, 255 } };

const uint8_t PROGMEM pixels00[] = {
  0X28, 0X28, 0X28, 0X28, 0X28, 0X28, 0X28, 0X28,
  0X28, 0X28, 0X28, 0X13, 0X00, 0X00, 0X28, 0X1F,
  0X17, 0X0E, 0X05, 0X00, 0X00, 0X07, 0X19, 0X28,
  0X28, 0X28, 0X18, 0X06, 0X28, 0X1A, 0X09, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X04, 0X0C,
  0X0A, 0X00, 0X28, 0X14, 0X01, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X28, 0X14, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X28, 0X1A,
  0X09, 0X00, 0X00, 0X00, 0X00, 0X08, 0X11, 0X20,
  0X28, 0X1B, 0X0D, 0X00, 0X28, 0X1F, 0X17, 0X0E,
  0X05, 0X0B, 0X1C, 0X28, 0X28, 0X27, 0X12, 0X02,
  0X00, 0X00, 0X28, 0X28, 0X28, 0X28, 0X28, 0X28,
  0X28, 0X28, 0X28, 0X13, 0X00, 0X00, 0X00, 0X00,
  0X28, 0X28, 0X28, 0X28, 0X28, 0X26, 0X23, 0X28,
  0X28, 0X27, 0X12, 0X02, 0X00, 0X00, 0X28, 0X28,
  0X28, 0X21, 0X10, 0X02, 0X00, 0X08, 0X11, 0X20,
  0X28, 0X28, 0X28, 0X22, 0X28, 0X28, 0X28, 0X15,
  0X03, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X28, 0X28, 0X28, 0X15, 0X03, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X28, 0X28, 0X28, 0X21, 0X10, 0X02, 0X00, 0X00,
  0X00, 0X00, 0X04, 0X0F, 0X1E, 0X25, 0X28, 0X28,
  0X28, 0X28, 0X28, 0X24, 0X1B, 0X16, 0X1D, 0X28,
  0X28, 0X28, 0X18, 0X06 };

typedef struct {
  uint8_t        type;    // PALETTE[1,4,8] or TRUECOLOR
  line_t         lines;   // Length of image (in scanlines)
  const uint8_t *palette; // -> PROGMEM color table (NULL if truecolor)
  const uint8_t *pixels;  // -> Pixel data in PROGMEM
} image;

const image PROGMEM images[] = {
  { PALETTE8 ,   14, (const uint8_t *)palette00, pixels00 }
};

#define NUM_IMAGES (sizeof(images) / sizeof(images[0]))
