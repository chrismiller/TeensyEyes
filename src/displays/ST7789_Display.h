#pragma once

#include <ST7789_t3.h>
#include <array>

#include "Display.h"

typedef struct {
  int8_t cs;           // Chip select pin, or -1.
  int8_t dc;            // DC pin.
  int8_t mosi;          // MOSI pin.
  int8_t sck;           // SCK pin.
  int8_t rst;           // reset pin, or -1 to disable reset.
  uint8_t rotation;     // The rotation value for the display (0-3).
  bool mirror;          // Mirror the display in the X direction.
  bool useFrameBuffer;  // Whether to use frame buffering.
  bool asyncUpdates;    // Whether to update the screen asynchronously (for better performance).
} ST7789_Config;

class ST7789_Display : public Display<ST7789_Display> {
private:
  ST7789_t3* display;
  bool asyncUpdates;
  int displayNum;

#ifdef SHOW_FPS
  uint32_t framesDrawn{};
  uint32_t fps{};
  elapsedMillis elapsed{};
#endif

public:
  /// Creates a generic wrapper for a 240x240 ST7789 TFT display screen.
  /// \param config the screen's configuration.
  ST7789_Display(const ST7789_Config &config);

  virtual ~ST7789_Display();

  inline void drawPixel(int16_t x, int16_t y, uint16_t color565) __attribute__((always_inline)) {
    display->drawPixel(x, y, color565);
  }

  inline void drawFastVLine(int16_t x, int16_t y, int16_t height, uint16_t color565) __attribute__((always_inline)) {
    display->drawFastVLine(x, y, height, color565);
  }

  inline uint16_t *frameBuffer() __attribute__((always_inline)) {
    return display->getFrameBuffer();
  }

  void markDirty(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    uint16_t *fb = display->getFrameBuffer();
    if (fb == nullptr) {
      return;
    }
    // The library keeps its changed-range tracking protected, so rewrite the two corner
    // pixels through drawPixel(); it stores the same values the framebuffer already
    // holds and expands the changed-range bounding box to cover the whole rectangle.
    const int16_t w = display->width();
    display->drawPixel(x0, y0, fb[y0 * w + x0]);
    display->drawPixel(x1, y1, fb[y1 * w + x1]);
  }

  void drawText(int16_t x, int16_t y, char *text);

  void update();

  bool isAvailable() const;
};
