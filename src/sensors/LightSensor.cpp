#include "LightSensor.h"

// Only read the sensor every 0.1s
constexpr uint32_t readFrequencyMs{100};

void LightSensor::readDamped(const std::function<void(float)> &processor) {
  if (isEnabled()) {
    auto const now = millis();
    if (now - lastReadTimeMs > readFrequencyMs) {
      lastReadTimeMs = now;
      uint32_t l = analogRead(pin);
      l = min(max(l, minReading), maxReading);
      float value = static_cast<float>(l - minReading) / static_cast<float>(maxReading - minReading);
      value = powf(value, curve);
      // Apply some smoothing
      value = value * 0.9f + previousValue * 0.1f;
      previousValue = value;
      processor(value);
    }
  }
}
