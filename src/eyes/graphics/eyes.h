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
  int32_t moveDuration{};
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
  BlinkState state;
  uint32_t duration;    // Duration of blink state (micros)
  uint32_t startTime;   // Time (micros) of last state change
  float blinkFactor;    // The most recent amount of blink [0..1] that was applied. 0 = not blinking, 1 = full blink
};

/// One-per-eye structure
struct Eye {
  Display *display; // -> OLED/TFT object
  /// The direction the eyes are looking, with mapRadius meaning straight ahead
  float x{};
  float y{};
  /// Current blink/wink state
  EyeBlink blink;
  // These hold the previous iteration of eyelid 'tracking' adjustments, so we can apply some dampening each frame
  float upperLidFactor{};
  float lowerLidFactor{};
};

struct PupilParams {
  const uint16_t color{};        // 16-bit 565 RGB, big-endian
  const uint16_t slitRadius{};   // Slit radius, in pixels. Use zero for a round pupil
  const float min{0.2};
  const float max{0.8};
};

struct IrisParams {
  const uint16_t radius{60};  // Iris radius, in pixels
  const Image texture{};
  const uint16_t color{};     // 16-bit 565 RGB, big-endian
  const uint16_t angle{};
  const uint16_t spin{};
};

struct ScleraParams {
  const Image texture{};
  const uint16_t color{};   // 16-bit 565 RGB, big-endian
  const uint16_t angle{};
  const uint16_t spin{};
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
};

struct PolarParams {
  const uint16_t mapRadius{240};  // Pixels
  const uint8_t *angle{};
  const uint8_t *distance{};
};

struct EyeParams {
  const uint16_t radius{120};     // Eye radius, in pixels
  const uint16_t backColor{};     // 16-bit 565 RGB, big-endian
  const bool tracking{false};     // Whether the eyelids 'track' the pupils or not
  const uint8_t *displacement{};
  const PupilParams pupil{};
  const IrisParams iris{};
  const ScleraParams sclera{};
  const EyelidParams eyelids{};
  const PolarParams polar{};
};