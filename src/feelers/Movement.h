#pragma once

#include <cmath>
#include <Arduino.h>

struct Target {
  // X/Y coords are in the range [-1, 1]
  float x;
  float y;
  uint32_t durationMs;
};

class Movement {
  protected:
  Target prev{};

  virtual Target nextTarget() = 0;

  float cubicEase(float x) {
    // https://easings.net/#easeInOutCubic
    return x < 0.5 ? 4 * x * x * x : 1 - std::pow(-2.0f * x + 2.0f, 3.0f) / 2.0f;
  }

  public:
  Target next() {
    prev = nextTarget();
    return prev;
  };

  /**
   * An easing function that takes a linearly interpolated value and returns an eased result.
   *
   * @param fraction a linearly interpolated value between 0 and 1.
   * @return an eased value between 0 and 1.
   */
  virtual float ease(float fraction) {
    // Default to linear
    return fraction;
  }
};

class StopMovement : public Movement {
  private:
  uint32_t duration{};
  public:
  StopMovement(uint32_t duration) : duration(duration) {}

  Target nextTarget() override {
    return Target{0.0f, 0.0f, duration};
  }
};

class RandomMovement : public Movement {
  private:
  static const int MIN_PAUSE_MS{50};
  static const int MAX_PAUSE_MS{1000};
  static const int MIN_MOVE_DURATION_MS{600};
  static const int MAX_MOVE_DURATION_MS{2000};
  bool inMotion{false};

  public:
  Target nextTarget() override;

  float ease(float x) override {
    // Easing function: 3*x^2-2*x^3, values in range 0.0 to 1.0
    // https://www.wolframalpha.com/input?i=3*x%5E2-2*x%5E3
    return x * x * (3 - 2 * x);
  }
};

class CircularMovement : public Movement {
  private:
  uint32_t msPerRotation{};
  bool clockwise{};
  public:
  CircularMovement(uint32_t msPerRotation, bool clockwise = false) : msPerRotation(msPerRotation),
                                                                     clockwise(clockwise) {}

  Target nextTarget() override;
};

class SidewaysMovement : public Movement {
  private:
  uint32_t msPerSweep{};
  bool inverted{};
  public:
  SidewaysMovement(uint32_t msPerSweep, bool inverted = false) : msPerSweep(msPerSweep), inverted(inverted) {}

  Target nextTarget() override;

  float ease(float fraction) override { return cubicEase(fraction); }
};

class VerticalMovement : public Movement {
  private:
  uint32_t msPerSweep{};
  bool inverted{};
  public:
  VerticalMovement(uint32_t msPerSweep, bool inverted = false) : msPerSweep(msPerSweep), inverted(inverted) {}

  Target nextTarget() override;

  float ease(float fraction) override { return cubicEase(fraction); }
};

class DiagonalMovement : public Movement {
  private:
  uint32_t msPerSweep{};
  bool inverted{};
  public:
  DiagonalMovement(uint32_t msPerSweep, bool inverted = false) : msPerSweep(msPerSweep), inverted(inverted) {}

  Target nextTarget() override;

  float ease(float fraction) override { return cubicEase(fraction); }
};

