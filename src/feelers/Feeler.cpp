#include "Feeler.h"

Feeler::Feeler() = default;

void Feeler::init(int lrPin, int udPin, Movement *m) {
  lrServo.attach(lrPin, 500, 2500);
  udServo.attach(udPin, 500, 2500);
  movement = m;
}

void Feeler::setMovement(Movement *m) {
  movement = m;
  lastUpdateTimeMs = 0;
}

void Feeler::update() {
  uint32_t t = millis();

  if (t - lastUpdateTimeMs <= UPDATE_RATE_MS) {
    // Don't update the servos too frequently
    return;
  }
  lastUpdateTimeMs = t;

  float prevX = currentX;
  float prevY = currentY;

  uint32_t dt = t - moveStartTimeMs;
  if (dt < target.durationMs) {
    // Interpolate and apply any easing function, to find the new position
    float delta = static_cast<float>(dt) / static_cast<float>(target.durationMs);
    delta = movement->ease(delta);
    currentX = startX + (target.x - startX) * delta;
    currentY = startY + (target.y - startY) * delta;
  } else {
    // We have reached the destination, find out where to move to next
    moveStartTimeMs = t;           // Save time of event
    currentX = startX = target.x;  // Save position
    currentY = startY = target.y;
    target = movement->next();
  }
  if (prevX != currentX) {
    int xAngle = static_cast<int>(currentX * 90.0f) + 90;
    lrServo.write(xAngle);
  }
  if (prevY != currentY) {
    int yAngle = static_cast<int>(currentY * 90.0f) + 90;
    udServo.write(yAngle);
  }
}
