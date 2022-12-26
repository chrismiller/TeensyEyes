#include <Wire.h>
#include "PersonSensor.h"

// The I2C address of the person sensor board.
constexpr uint8_t I2C_ADDRESS{0x62};

bool PersonSensor::read() {
  if (timeSinceSampledMs < SAMPLE_TIME_MS) {
    return false;
  }
  Wire.requestFrom(I2C_ADDRESS, sizeof(person_sensor_results_t));
  if (Wire.available() != sizeof(person_sensor_results_t)) {
    return false;
  }
  auto *results_bytes = (uint8_t *) (&results);
  for (unsigned int i = 0; i < sizeof(person_sensor_results_t); ++i) {
    results_bytes[i] = Wire.read();
  }
  timeSinceSampledMs = 0;
  return true;
}

void PersonSensor::writeReg(PersonSensor::Reg reg, uint8_t value) {
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(static_cast<uint8_t>(reg));
  Wire.write(value);
  Wire.endTransmission();
}
