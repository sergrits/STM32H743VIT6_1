#include "st7735.h"
#include "main.h"
#include <string.h>

static SPI_HandleTypeDef *st_hspi;

static uint16_t _width = ST7735_WIDTH;
static uint16_t _height = ST7735_HEIGHT;
static uint8_t rotation = 0;

// Many 0.96" 80x160 ST7735 modules need offsets.
// If your colors/position are shifted, tweak these:
static uint8_t colstart = 0;
static uint8_t rowstart = 0;

static inline void CS_L(void){ HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET); }
static inline void CS_H(void){ HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET); }
static inline void DC_L(void){ HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET); }
static inline void DC_H(void){ HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET); }

static void write_cmd(uint8_t cmd)
{
  DC_L();
  CS_L();
  HAL_SPI_Transmit(st_hspi, &cmd, 1, HAL_MAX_DELAY);
  CS_H();
}

static void write_data(const uint8_t *data, uint16_t len)
{
  DC_H();
  CS_L();
  HAL_SPI_Transmit(st_hspi, (uint8_t*)data, len, HAL_MAX_DELAY);
  CS_H();
}

static void write_data8(uint8_t v){ write_data(&v, 1); }

static void write_data16(uint16_t v)
{
  uint8_t d[2] = { (uint8_t)(v >> 8), (uint8_t)(v & 0xFF) };
  write_data(d, 2);
}

static void set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  x0 += colstart; x1 += colstart;
  y0 += rowstart; y1 += rowstart;

  write_cmd(0x2A); // CASET
  uint8_t xa[4] = { (uint8_t)(x0>>8), (uint8_t)x0, (uint8_t)(x1>>8), (uint8_t)x1 };
  write_data(xa, 4);

  write_cmd(0x2B); // RASET
  uint8_t ya[4] = { (uint8_t)(y0>>8), (uint8_t)y0, (uint8_t)(y1>>8), (uint8_t)y1 };
  write_data(ya, 4);

  write_cmd(0x2C); // RAMWR
}

void ST7735_SetRotation(uint8_t r)
{
  rotation = r & 3;
  write_cmd(0x36); // MADCTL
  uint8_t madctl = 0x00;

  // Default: RGB order (bit3 = 0). If colors are swapped (BGR), set bit3.
  // madctl |= 0x08;

  switch (rotation)
  {
    case 0: madctl |= 0x00; _width = ST7735_WIDTH;  _height = ST7735_HEIGHT; break;
    case 1: madctl |= 0x60; _width = ST7735_HEIGHT; _height = ST7735_WIDTH;  break;
    case 2: madctl |= 0xC0; _width = ST7735_WIDTH;  _height = ST7735_HEIGHT; break;
    case 3: madctl |= 0xA0; _width = ST7735_HEIGHT; _height = ST7735_WIDTH;  break;
  }
  write_data8(madctl);
}

void ST7735_Init(SPI_HandleTypeDef *hspi)
{
  st_hspi = hspi;

  // Make sure BL is on
  HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);

  HAL_Delay(120);

  // SWRESET
  write_cmd(0x01);
  HAL_Delay(150);

  // SLPOUT
  write_cmd(0x11);
  HAL_Delay(150);

  // COLMOD: 16-bit color
  write_cmd(0x3A);
  write_data8(0x05);
  HAL_Delay(10);

  // Some modules need inversion ON for correct colors
  write_cmd(0x21); // INVON
  HAL_Delay(10);

  ST7735_SetRotation(0);

  // DISPON
  write_cmd(0x29);
  HAL_Delay(100);
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
  if (x >= _width || y >= _height) return;
  set_addr_window(x, y, x, y);
  write_data16(color);
}

void ST7735_FillScreen(uint16_t color)
{
  set_addr_window(0, 0, _width - 1, _height - 1);

  uint8_t hi = color >> 8;
  uint8_t lo = color & 0xFF;

  // Chunked fill to avoid huge buffers
  uint8_t buf[64];
  for (int i = 0; i < (int)sizeof(buf); i += 2) { buf[i] = hi; buf[i+1] = lo; }

  uint32_t pixels = (uint32_t)_width * (uint32_t)_height;
  while (pixels)
  {
    uint16_t chunk_pixels = (pixels > (sizeof(buf)/2)) ? (sizeof(buf)/2) : pixels;
    write_data(buf, chunk_pixels * 2);
    pixels -= chunk_pixels;
  }
}

// 5x7 font (ASCII 32..127), minimal subset to keep file short.
// If you want full table — скажи, добавлю полностью.
static const uint8_t font5x7[][5] = {
  // ' ' (32)
  {0x00,0x00,0x00,0x00,0x00},
  // '!' (33)
  {0x00,0x00,0x5F,0x00,0x00},
  // '"' (34)
  {0x00,0x07,0x00,0x07,0x00},
  // '#' (35)
  {0x14,0x7F,0x14,0x7F,0x14},
  // '$' (36)
  {0x24,0x2A,0x7F,0x2A,0x12},
  // '%' (37)
  {0x23,0x13,0x08,0x64,0x62},
  // '&' (38)
  {0x36,0x49,0x55,0x22,0x50},
  // ''' (39)
  {0x00,0x05,0x03,0x00,0x00},
  // '(' (40)
  {0x00,0x1C,0x22,0x41,0x00},
  // ')' (41)
  {0x00,0x41,0x22,0x1C,0x00},
  // '*' (42)
  {0x14,0x08,0x3E,0x08,0x14},
  // '+' (43)
  {0x08,0x08,0x3E,0x08,0x08},
  // ',' (44)
  {0x00,0x50,0x30,0x00,0x00},
  // '-' (45)
  {0x08,0x08,0x08,0x08,0x08},
  // '.' (46)
  {0x00,0x60,0x60,0x00,0x00},
  // '/' (47)
  {0x20,0x10,0x08,0x04,0x02},
  // '0'..'9' (48..57)
  {0x3E,0x51,0x49,0x45,0x3E},
  {0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46},
  {0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10},
  {0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30},
  {0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36},
  {0x06,0x49,0x49,0x29,0x1E},
  // ':' (58)
  {0x00,0x36,0x36,0x00,0x00},
  // ';' (59)
  {0x00,0x56,0x36,0x00,0x00},
  // '<' (60)
  {0x08,0x14,0x22,0x41,0x00},
  // '=' (61)
  {0x14,0x14,0x14,0x14,0x14},
  // '>' (62)
  {0x00,0x41,0x22,0x14,0x08},
  // '?' (63)
  {0x02,0x01,0x51,0x09,0x06},
  // '@' (64)
  {0x32,0x49,0x79,0x41,0x3E},
  // 'A'..'Z' (65..90)
  {0x7E,0x11,0x11,0x11,0x7E},
  {0x7F,0x49,0x49,0x49,0x36},
  {0x3E,0x41,0x41,0x41,0x22},
  {0x7F,0x41,0x41,0x22,0x1C},
  {0x7F,0x49,0x49,0x49,0x41},
  {0x7F,0x09,0x09,0x09,0x01},
  {0x3E,0x41,0x49,0x49,0x7A},
  {0x7F,0x08,0x08,0x08,0x7F},
  {0x00,0x41,0x7F,0x41,0x00},
  {0x20,0x40,0x41,0x3F,0x01},
  {0x7F,0x08,0x14,0x22,0x41},
  {0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F},
  {0x7F,0x04,0x08,0x10,0x7F},
  {0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x09,0x09,0x06},
  {0x3E,0x41,0x51,0x21,0x5E},
  {0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31},
  {0x01,0x01,0x7F,0x01,0x01},
  {0x3F,0x40,0x40,0x40,0x3F},
  {0x1F,0x20,0x40,0x20,0x1F},
  {0x7F,0x20,0x18,0x20,0x7F},
  {0x63,0x14,0x08,0x14,0x63},
  {0x03,0x04,0x78,0x04,0x03},
  {0x61,0x51,0x49,0x45,0x43},
};

void ST7735_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg)
{
  if (c < 32 || c > 90) c = '?';
  const uint8_t *glyph = font5x7[c - 32];

  for (uint8_t i = 0; i < 5; i++)
  {
    uint8_t line = glyph[i];
    for (uint8_t j = 0; j < 8; j++)
    {
      uint16_t px = x + i;
      uint16_t py = y + j;
      if (px >= _width || py >= _height) continue;
      ST7735_DrawPixel(px, py, (line & 0x01) ? color : bg);
      line >>= 1;
    }
  }
}

void ST7735_DrawString(uint16_t x, uint16_t y, const char *s, uint16_t color, uint16_t bg)
{
  while (*s)
  {
    ST7735_DrawChar(x, y, *s++, color, bg);
    x += 6;
    if (x + 5 >= _width) { x = 0; y += 8; }
    if (y + 7 >= _height) break;
  }
}
