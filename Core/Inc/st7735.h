#ifndef ST7735_H
#define ST7735_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define ST7735_WIDTH  80
#define ST7735_HEIGHT 160

// RGB565 colors
#define ST7735_BLACK   0x0000
#define ST7735_WHITE   0xFFFF
#define ST7735_RED     0xF800
#define ST7735_GREEN   0x07E0
#define ST7735_BLUE    0x001F
#define ST7735_YELLOW  0xFFE0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F

void ST7735_Init(SPI_HandleTypeDef *hspi);
void ST7735_SetRotation(uint8_t r);
void ST7735_FillScreen(uint16_t color);
void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7735_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg);
void ST7735_DrawString(uint16_t x, uint16_t y, const char *s, uint16_t color, uint16_t bg);

#endif******************* (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
