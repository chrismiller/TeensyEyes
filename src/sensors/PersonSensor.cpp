#include <Wire.h>
#include "PersonSensor.h"

PersonSensor::PersonSensor(TwoWire &wire): wire(wire) {}

bool PersonSensor::isPresent() {
  wire.beginTransmission(I2C_ADDRESS);
  return wire.endTransmission() == 0;
}

bool PersonSensor::read() {
  if (timeSinceSampledMs < SAMPLE_TIME_MS) {
    return false;
  }
  wire.requestFrom(I2C_ADDRESS, sizeof(person_sensor_results_t));
  if (wire.available() != sizeof(person_sensor_results_t)) {
    return false;
  }
  auto *results_bytes = (uint8_t *) (&results);
  for (unsigned int i = 0; i < sizeof(person_sensor_results_t); ++i) {
    results_bytes[i] = Wire.read();
  }
  timeSinceSampledMs = 0;
  if (numFacesFound() > 0) {
    lastDetectionTimeMs = 0;
  }
  return true;
}

void PersonSensor::writeReg(PersonSensor::Reg reg, uint8_t value) {
  wire.beginTransmission(I2C_ADDRESS);
  wire.write(static_cast<uint8_t>(reg));
  wire.write(value);
  wire.endTransmission();
}
