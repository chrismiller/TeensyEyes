#pragma once

#include "eyes/EyeController.h"
#include "displays/GC9A01A_Display.h"


class EyeTest {
private:
  bool longGaze{};

public:
  EyeController<2, GC9A01A_Display> *controller{};

  void setup();

  void drawSingleFrame();

  void next() const;

  void previous() const;

  void blink() const;

  void autoMove() const;

  void autoPupils() const;

  void wink(size_t eyeIndex) const;

  void setMaxGazeMs(uint32_t maxGazeMs);
};
