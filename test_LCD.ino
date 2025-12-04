#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// Màu RGB565
#ifndef BLACK
#define BLACK 0x0000
#endif
#ifndef RED
#define RED 0xF800
#endif
#ifndef GREEN
#define GREEN 0x07E0
#endif
#ifndef BLUE
#define BLUE 0x001F
#endif

#define TFT_WIDTH 480
#define TFT_HEIGHT 272
#define TFT_ORIENTATION 2 // xoay ngang, giống ví dụ chính thức

#define GFX_BL 1 // chân điều khiển backlight

// Bus QSPI của JC4827W543 (theo repo chính thức)
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    45 /* cs */,
    47 /* sck */,
    21 /* d0 */,
    48 /* d1 */,
    40 /* d2 */,
    39 /* d3 */
);

// Driver NV3041A, KHÔNG dùng Canvas
Arduino_GFX *gfx = new Arduino_NV3041A(
    bus,
    GFX_NOT_DEFINED, // RST
    TFT_ORIENTATION, // rotation
    true             // IPS
);

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("JC4827W543 NV3041A simple test");

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH); // bật đèn nền

  if (!gfx->begin())
  {
    Serial.println("gfx->begin() FAILED!");
    while (1)
    {
      delay(1000);
    }
  }

  gfx->fillScreen(BLACK);
  delay(1000);
  gfx->fillScreen(RED);
  delay(1000);
  gfx->fillScreen(GREEN);
  delay(1000);
  gfx->fillScreen(BLUE);
  delay(1000);
}

void loop()
{
  // không làm gì
}