#pragma once

#include <GC9A01A_t3n.h>
#include <array>

#include "Display.h"

#define RGBColor(r, g, b) GC9A01A_t3n::Color565(r, g, b)

typedef struct {
  int8_t cs;            // Chip select pin.
  int8_t dc;            // DC pin.
  int8_t mosi;          // MOSI pin.
  int8_t sck;           // SCK pin.
  int8_t rst;           // reset pin, or -1 to disable reset.
  uint8_t rotation;     // The rotation value for the display (0-3).
  bool mirror;          // Mirror the display in the X direction.
  bool useFrameBuffer;  // Whether to use frame buffering.
  bool asyncUpdates;    // Whether to update the screen asynchronously (for better performance).
} GC9A01A_Config;

class GC9A01A_Display : public Display<GC9A01A_Display> {
private:
  GC9A01A_t3n* display;
  bool asyncUpdates;
  int displayNum;

#ifdef SHOW_FPS
  uint32_t framesDrawn{};
  uint32_t fps{};
  elapsedMillis elapsed{};
#endif

public:
  /// Creates a generic wrapper for a 240x240 GC9A01A round display screen.
  /// \param config the screen's configuration.
  /// \param spiSpeed the speed of the SPI bus. For maximum performance, set this as high as you can get
  /// away with. It will depend on the displays themselves, wire lengths, shielding/interference etc. My
  /// setup works up to about 90,000,000. At 100,000,000 I start seeing corruption on the displays.
  GC9A01A_Display(const GC9A01A_Config &config, const uint32_t spiSpeed = 30'000'000);

  virtual ~GC9A01A_Display();

  void drawPixel(int16_t x, int16_t y, uint16_t color565);

  void drawFastVLine(int16_t x, int16_t y, int16_t height, uint16_t color565);

  void drawText(int16_t x, int16_t y, char *text);

  void update();

  bool isAvailable() const;
};
