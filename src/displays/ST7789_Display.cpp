#include "ST7789_Display.h"

ST7789_t3 *createDisplay(const ST7789_Config &config) {
  if (config.cs >= 0) {
    pinMode(config.cs, OUTPUT);
    digitalWrite(config.cs, HIGH); // Deselect them all
  }
  return new ST7789_t3(config.cs, config.dc, config.mosi, config.sck, config.rst);
}

ST7789_Display::ST7789_Display(const ST7789_Config &config) :
    display(createDisplay(config)), asyncUpdates(config.asyncUpdates) {
  static size_t displayNum{};
  Serial.print(F("Init ST7789 display #"));
  Serial.print(displayNum);
  Serial.print(F(": rotate="));
  Serial.print(config.rotation);
  Serial.print(F(", mirror="));
  Serial.println(config.mirror);
  if (config.cs < 0) {
    // Try to handle the ST7789 displays without CS pins
    display->init(240, 240, SPI_MODE2);
  } else {
    display->init();
  }
  display->setRotation(config.rotation);
  if (config.mirror) {
    const std::array<uint8_t, 4> mirrorTFT{0x80, 0x20, 0x40, 0xE0}; // Mirror + rotate
    display->sendCommand(ST7735_MADCTL, &mirrorTFT.at(config.rotation & 3), 1);
  }

  if (config.useFrameBuffer) {
    Serial.print(displayNum);
    Serial.print(F(": useFrameBuffer() "));
    if (!display->useFrameBuffer(true)) {
      Serial.println(F("failed"));
    } else {
      Serial.println(F("OK"));
    }
  }
  Serial.println(F("Success"));
  this->displayNum = displayNum;
  displayNum++;
}

ST7789_Display::~ST7789_Display() {
  // TODO: this looks like a bug in ST7735_t3, it is meant to have a virtual destructor.
  //  about the best we can do is just free the frame buffer :-/
  // delete display;
  display->freeFrameBuffer();
}

void ST7789_Display::drawPixel(int16_t x, int16_t y, uint16_t color565) {
  display->drawPixel(x, y, color565);
}

void ST7789_Display::drawText(int16_t x, int16_t y, char *text) {
  display->setCursor(x, y);
  display->setTextSize(2);
  display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display->print(text);
}

void ST7789_Display::update() {

#ifdef SHOW_FPS
  // A per-display FPS counter
  if (elapsed >= 1000L) {
    fps = framesDrawn;
    framesDrawn = 0;
    elapsed = 0;
  }
  display->setTextSize(2);
  display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display->drawNumber(fps, 110, 110);
  framesDrawn++;
#endif

  if (asyncUpdates) {
    if (!display->updateScreenAsync()) {
      Serial.print(F("updateScreenAsync() failed for display "));
      Serial.println(displayNum);
    }
  } else {
    display->updateScreen();
  }
}

bool ST7789_Display::isAvailable() const {
  return !display->asyncUpdateActive();
}
