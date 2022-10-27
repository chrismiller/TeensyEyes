#pragma once

#include <Arduino.h>

// LARGE_EYES are designed for 240x240 screens
#define LARGE_EYES
constexpr uint16_t screenWidth = 240;
constexpr uint16_t screenHeight = 240;

struct Image {
  const uint16_t *data{};
  const uint16_t width{};
  const uint16_t height{};

  inline uint16_t get(uint16_t x, uint16_t y) const __attribute__((always_inline)) {
    return data[y * width + x];
  }
};

/// Global state that applies to all eyes (not per-eye)
struct OverallState {
  /// Whether the eyes are currently in motion
  bool inMotion{false};
  float eyeOldX{};
  float eyeOldY{};
  float eyeNewX{};
  float eyeNewY{};
  uint32_t moveStartTime{};
  uint32_t moveDuration{};
  uint32_t lastSaccadeStop{};
  uint32_t saccadeInterval{};
  int fixate{7};
  uint32_t timeToNextBlink{};
  uint32_t timeOfLastBlink{};
};

enum class BlinkState {
  NotBlinking, BlinkClosing, BlinkOpening
};

/// A simple state machine is used to control eye blinks/winks
struct EyeBlink {
  BlinkState state{BlinkState::NotBlinking};
  uint32_t duration{};    // Duration of blink state (micros)
  uint32_t startTime{};   // Time (micros) of last state change
  float blinkFactor{};    // The most recent amount of blink [0..1] that was applied. 0 = not blinking, 1 = full blink
};

struct PupilParams {
  const uint16_t color{};        // 16-bit 565 RGB, big-endian
  const uint16_t slitRadius{};   // Slit radius, in pixels. Use zero for a round pupil
  const float min{0.2};
  const float max{0.8};
};

struct IrisParams {
  const uint16_t radius{60};    // Iris radius, in pixels
  const Image texture{};
  const uint16_t color{};       // 16-bit 565 RGB, big-endian
  const uint16_t startAngle{};  // Initial rotation, 0-1023 CCW
  const float spin{};           // RPM * 1024.0
  const uint16_t iSpin{};       // Per-frame fixed integer spin. If set, this overrides 'spin'
  const uint16_t mirror{};      // 0 = normal, 1023 = flip the X axis

  bool hasTexture() const {
    return texture.data != nullptr;
  }
};

struct ScleraParams {
  const Image texture{};
  const uint16_t color{};       // 16-bit 565 RGB, big-endian
  const uint16_t startAngle{};  // Initial rotation, 0-1023 CCW
  const float spin{};           // RPM * 1024.0
  const uint16_t iSpin{};       // Per-frame fixed integer spin. If set, this overrides 'spin'
  const uint16_t mirror{};      // 0 = normal, 1023 = flip the X axis

  bool hasTexture() const {
    return texture.data != nullptr;
  }
};

struct EyelidParams {
  const uint8_t *upper{};
  const uint8_t *lower{};
  const uint16_t color{};   // 16-bit 565 RGB, big-endian

  inline uint8_t upperOpen(uint8_t x) const __attribute__((always_inline)) {
    return upper[x * 2];
  }

  inline uint8_t upperClosed(uint8_t x) const __attribute__((always_inline)) {
    return upper[x * 2 + 1];
  }

  inline uint8_t lowerOpen(uint8_t x) const __attribute__((always_inline)) {
    return lower[x * 2 + 1];
  }

  inline uint8_t lowerClosed(uint8_t x) const __attribute__((always_inline)) {
    return lower[x * 2];
  }

  inline uint8_t upperThreshold(uint8_t x, uint8_t y) const __attribute__((always_inline)) {
    const uint8_t start = upperOpen(x);
    const uint8_t end = upperClosed(x);
    return y <= start ? 0 : y >= end ? 255 : (y - start) * 256 / (end - start);
  }

  inline uint8_t lowerThreshold(uint8_t x, uint8_t y) const __attribute__((always_inline)) {
    const uint8_t start = lowerOpen(x);
    const uint8_t end = lowerClosed(x);
    return y <= start ? 255 : y >= end ? 0 : (end - y) * 256 / (end - start);
  }

  /// Compute the Y coordinate of the upper eyelid at a given X coordinate.
  /// \param x the X location in pixels.
  /// \param proportion the proportion the eyelid is open. 0 = fully closed, 1 = fully open.
  /// \return  the Y coordinate in pixels of the edge of the top eyelid.
  inline uint8_t upperLid(uint8_t x, float proportion) const __attribute__((always_inline)) {
    const uint8_t start = upperOpen(x);
    const uint8_t end = upperClosed(x);
    return (uint8_t) ((float) end - proportion * (float) (end - start));
  }

  /// Compute the Y coordinate of the lower eyelid at a given X coordinate.
  /// \param x the X location in pixels.
  /// \param proportion the proportion the eyelid is open. 0 = fully closed, 1 = fully open.
  /// \return  the Y coordinate in pixels of the edge of the bottom eyelid.
  inline uint8_t lowerLid(uint8_t x, float proportion) const __attribute__((always_inline)) {
    const uint8_t start = lowerClosed(x);
    const uint8_t end = lowerOpen(x);
    return (uint8_t) ((float) start + proportion * (float) (end - start));
  }
};

struct PolarParams {
  const uint16_t mapRadius{240};  // Pixels
  const uint8_t *angle{};
  const uint8_t *distance{};
};

struct EyeDefinition {
  const uint16_t radius{120};     // Eye radius, in pixels
  const uint16_t backColor{};     // 16-bit 565 RGB, big-endian
  const bool tracking{false};     // Whether the eyelids 'track' the pupils or not
  const float squint{};
  const uint8_t *displacement{};
  const PupilParams pupil{};
  const IrisParams iris{};
  const ScleraParams sclera{};
  const EyelidParams eyelids{};
  const PolarParams polar{};
};

/// One-per-eye structure. Mutable, holding the current state of an eye/display.
struct Eye {
  Display *display{}; // -> OLED/TFT object
  EyeDefinition *definition{};
  /// The direction the eyes are looking, with mapRadius meaning straight ahead
  float x{};
  float y{};
  uint16_t currentIrisAngle{};
  uint16_t currentScleraAngle{};
  /// Current blink/wink state
  EyeBlink blink{};
  // These hold the previous iteration of eyelid 'tracking' adjustments, so we can apply some dampening each frame
  float upperLidFactor{};
  float lowerLidFactor{};
  bool drawAll{};
};