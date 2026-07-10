#pragma once

#include <Arduino.h>

//#define SHOW_FPS

template <typename T>
class Display {
public:
  inline void drawPixel(int16_t x, int16_t y, uint16_t color565) const __attribute__((always_inline)) {
    static_cast<T&>(*this).drawPixel(x, y, color565);
  }

  inline void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color565) const __attribute__((always_inline)) {
    static_cast<T&>(*this).drawFastVLine(x, y, h, color565);
  }

  /// Direct access to the display's 16-bit (565 RGB) framebuffer, or nullptr if the display
  /// is not using a framebuffer. Pixels are stored row-major, screenWidth to a row. Writes
  /// made through this pointer bypass the display's dirty-region tracking, so the caller
  /// must report what it wrote via markDirty() or the pixels may never reach the screen.
  inline uint16_t *frameBuffer() __attribute__((always_inline)) {
    return static_cast<T&>(*this).frameBuffer();
  }

  /// Marks the rectangle with the given (inclusive) corner coordinates as changed, so it
  /// will be included in the next update(). Required after writing via frameBuffer().
  inline void markDirty(int16_t x0, int16_t y0, int16_t x1, int16_t y1) __attribute__((always_inline)) {
    static_cast<T&>(*this).markDirty(x0, y0, x1, y1);
  }

  void drawText(uint16_t x, uint16_t y, char *text) {
    static_cast<T&>(*this).drawText(x, y, text);
  }

  void update() {
    static_cast<T&>(*this).update();
  }

  /// Whether the display is available for drawing to.
  /// \return true if the display is ready for use, false otherwise.
  bool isAvailable() {
    return static_cast<T&>(*this).isAvailable();
  }
};
