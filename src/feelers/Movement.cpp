#pragma once

#include "Movement.h"

Target RandomMovement::nextTarget() {
  if (inMotion) {
    inMotion = false;
    uint32_t durationMs = random(MIN_PAUSE_MS, MAX_PAUSE_MS);
    return Target{
        prev.x,
        prev.y,
        durationMs
    };
  } else {
    inMotion = true;
    float targetX = static_cast<float>(random(-1000, 1000)) / 1000.f;
    float targetY = static_cast<float>(random(-1000, 1000)) / 1000.f;
    uint32_t durationMs = random(MIN_MOVE_DURATION_MS, MAX_MOVE_DURATION_MS);
    return Target{
        targetX,
        targetY,
        durationMs
    };
  }
}

Target CircularMovement::nextTarget() {
  uint32_t now = millis();
  float fraction = static_cast<float>(now % msPerRotation) / static_cast<float>(msPerRotation);
  if (clockwise) {
    fraction = 1.0f - fraction;
  }
  fraction *= 2 * PI;
  float x = sin(fraction);
  float y = cos(fraction);

  // Perform 12 moves per rotation to approximate a circle
  return Target{x, y, msPerRotation / 12};
}

Target SidewaysMovement::nextTarget() {
  float xTarget = prev.x == 1.0f ? -1.0f : 1.0f;
  return Target{inverted ? -xTarget : xTarget, 0.0f, msPerSweep};
}

Target VerticalMovement::nextTarget() {
  float yTarget = prev.y == 1.0f ? -1.0f : 1.0f;
  return Target{0.0f, inverted ? -yTarget : yTarget, msPerSweep};
}

Target DiagonalMovement::nextTarget() {
  float target = prev.x == 1.0f ? -1.0f : 1.0f;
  return Target{target, inverted ? -target : target, msPerSweep};
}

