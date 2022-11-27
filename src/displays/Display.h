#pragma once

#include <Arduino.h>

// #define SHOW_FPS

template <typename T>
class Display {
public:
  inline void drawPixel(uint16_t x, uint16_t y, uint16_t color565) {
    static_cast<T&>(*this).drawPixel(x, y, color565);
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
