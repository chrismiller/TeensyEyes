#pragma once

#include <Arduino.h>
#include <functional>

class LightSensor {
private:
  int8_t pin{};
  uint32_t minReading{};
  uint32_t maxReading{};
  float curve{};
  uint32_t lastReadTimeMs{};
  float previousValue{0.5f};

public:
  explicit LightSensor(int8_t pin, uint32_t minReading = 0, uint32_t maxReading = 1023, float curve = 1.0f)
      : pin(pin), minReading(minReading), maxReading(maxReading), curve(curve) {};

  bool isEnabled() const {
    return pin >= 0;
  }

  void readDamped(std::function<void(float)> const& processor);
};
