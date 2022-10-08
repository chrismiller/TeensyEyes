#pragma once

#include <Arduino.h>

constexpr const uint16_t screenWidth = 240;
constexpr const uint16_t screenHeight = 240;

struct Image {
private:
  const uint16_t *data;
public:
  const uint16_t width;
  const uint16_t height;

  Image(const uint16_t *data, const uint16_t width, const uint16_t height) : data(data), width(width), height(height) {}

  inline uint16_t get(uint16_t x, uint16_t y) const __attribute__((always_inline)) {
    return data[y * width + x];
  }
};

struct EyeParams {
private:
public:
  const uint16_t pupilColor;   // 16-bit 565 RGB, big-endian
  const uint16_t backColor;    // 16-bit 565 RGB, big-endian
  const uint16_t irisSize;
  const uint16_t irisMin{120};    // Iris size (0-1023) in the brightest light
  const uint16_t irisMax{900};    // Iris size (0-1023) in the darkest light
  const uint16_t eyeRadius{120};  // Pixels
  const uint16_t mapRadius{240};  // Pixels
  const Image iris;
  [[deprecated("Replaced with non-polar sclera implementation")]]      // TODO: deprecated
  const Image polarSclera;
  const Image sclera;
  [[deprecated("Replaced with polarAngle and polarDist maps")]]        // TODO: deprecated
  const Image polar;
  const uint8_t *polarAngle;
  const int8_t *polarDist;
  const uint8_t *disp;
  const uint8_t *upperLid;
  const uint8_t *lowerLid;

  EyeParams(const uint16_t pupilColor, const uint16_t backColor, const uint16_t irisSize, const uint16_t irisMin,
            const uint16_t irisMax, const uint16_t eyeRadius, const uint16_t mapRadius, const Image &iris,
            const Image polarSclera, const Image &sclera, const Image &polar, const uint8_t *polarAngle,
            const int8_t *polarDist, const uint8_t *disp, const uint8_t *upperLid, const uint8_t *lowerLid) :
      pupilColor(pupilColor), backColor(backColor), irisSize(irisSize), irisMin(irisMin), irisMax(irisMax),
      eyeRadius(eyeRadius), mapRadius(mapRadius), iris(iris), polarSclera(polarSclera), sclera(sclera),
      polar(polar), polarAngle(polarAngle), polarDist(polarDist), disp(disp), upperLid(upperLid), lowerLid(lowerLid) {}
};