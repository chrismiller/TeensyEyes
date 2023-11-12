#pragma once

// The G++ with Teensydunio 1.59 beta 3 adds a new warning. This silences it.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

#include <Arduino.h>

constexpr uint16_t screenWidth = 240;
constexpr uint16_t screenHeight = 240;

struct Image {
  const uint16_t *data{};
  const uint16_t width{};
  const uint16_t height{};

  inline uint16_t get(uint32_t x, uint32_t y) const __attribute__((always_inline)) {
    return data[y * width + x];
  }
};

/// Global state that applies to all eyes (not per-eye)
struct OverallState {
  /// Whether the eyes are currently in motion
  bool inMotion{};
  float eyeOldX{};
  float eyeOldY{};
  float eyeNewX{};
  float eyeNewY{};
  uint32_t moveStartTimeMs{};
  uint32_t moveDurationMs{};
  uint32_t lastSaccadeStopMs{};
  uint32_t saccadeIntervalMs{};
  uint32_t timeToNextBlinkMs{};
  uint32_t timeOfLastBlinkMs{};

  // Variables for keeping track of the pupil size
  uint16_t irisFrame{};
  float pupilAmount{0.5f};  // 0 (smallest) to 1 (largest)
  bool resizing{};
  float resizeStart{};
  float resizeTarget{};
  uint32_t resizeStartTimeMs{};
  uint32_t resizeDurationMs{};

  int fixate{7};
};

enum class BlinkState {
  NotBlinking, BlinkClosing, BlinkOpening
};

/// A simple state machine is used to control eye blinks/winks
struct EyeBlink {
  BlinkState state{BlinkState::NotBlinking};
  uint32_t durationMs{};  // Duration of blink state
  uint32_t startTimeMs{}; // Time of last state change
  float blinkFactor{};    // The most recent amount of blink [0..1] that was applied. 0 = not blinking, 1 = full blink
};

struct PupilParams {
  /// 16-bit 565 RGB, big-endian
  const uint16_t color{};
  /// Slit radius, in pixels. Use zero for a round pupil
  const uint16_t slitRadius{};
  /// The minimum size of the pupil, as a ratio of the iris size
  const float min{0.2};
  /// The maximum size of the pupil, as a ratio of the iris size
  const float max{0.8};
};

struct IrisParams {
  /// Iris radius, in pixels
  const uint16_t radius{60};
  /// The iris texture image. Optional.
  const Image texture{};
  /// The iris color, used if no texture is set. 16-bit 565 RGB, big-endian
  const uint16_t color{};
  /// Initial rotation, 0-1023 CCW
  const uint16_t startAngle{};
  /// How quickly to rotate the iris image (RPM * 1024.0). Zero disables any animation.
  const float spin{};
  /// Per-frame fixed integer spin, instead of time-based. If set, this overrides 'spin'.
  const uint16_t iSpin{};
  /// Controls mirroring of the iris image. 0 = normal, 1023 = flip the X axis.
  const uint16_t mirror{};

  bool hasTexture() const {
    return texture.data != nullptr;
  }
};

struct ScleraParams {
  /// The sclera texture image. Optional.
  const Image texture{};
  /// The sclera color, used if no texture is set. 16-bit 565 RGB, big-endian.
  const uint16_t color{};
  /// Initial rotation, 0-1023 CCW.
  const uint16_t startAngle{};
  /// How quickly to rotate the sclera image (RPM * 1024.0). Zero disables any animation.
  const float spin{};
  /// Per-frame fixed integer spin, instead of time-based. If set, this overrides 'spin'.
  const uint16_t iSpin{};
  /// Controls mirroring of the sclera image. 0 = normal, 1023 = flip the X axis.
  const uint16_t mirror{};

  bool hasTexture() const {
    return texture.data != nullptr;
  }
};

struct EyelidParams {
  /// An array of bytes that specify the top and bottom limits of the upper eyelid at each X coordinate.
  const uint8_t *upper{};
  /// An array of bytes that specify the top and bottom limits of the lower eyelid at each X coordinate.
  const uint8_t *lower{};
  /// The color of the eyelid. 16-bit 565 RGB, big-endian.
  const uint16_t color{};

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
  uint16_t mapRadius{240};  // Pixels
  const uint8_t *angle{};
  const uint8_t *distance{};
};

struct EyeDefinition {
  char name[16];            // The name of the eye, useful for logging/debugging
  uint16_t radius{120};     // Eye radius, in pixels
  uint16_t backColor{};     // 16-bit 565 RGB, big-endian
  bool tracking{false};     // Whether the eyelids 'track' the pupils or not
  float squint{};
  const uint8_t *displacement{};
  PupilParams pupil{};
  IrisParams iris{};
  ScleraParams sclera{};
  EyelidParams eyelids{};
  PolarParams polar{};
};

/// One-per-eye structure. Mutable, holding the current state of an eye/display.
template <typename Disp>
struct Eye {
  Disp *display{};        // A Display implementation
  const EyeDefinition *definition{};
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

template <typename Disp>
struct DisplayDefinition {
  Disp *display{};        // A Display implementation
  EyeDefinition &definition;
};

#pragma GCC diagnostic pop
