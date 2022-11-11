// Define if you wish to debug memory usage.  Only works on T4.x
//#define DEBUG_MEMORY

#include <SPI.h>
#include <array>

#include "EyeTest.h"
#include "util/logging.h"

static EyeTest eyes{};

constexpr uint32_t EYE_DURATION_MS{4'000};

// TFT: use max SPI (clips to 12 MHz on M0)
constexpr uint32_t SPI_FREQUENCY{48'000'000};


/// INITIALIZATION -- runs once at startup ----------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  delay(200);
  DumpMemoryInfo();
  Serial.println("Init");
  Serial.flush();
  randomSeed(analogRead(A3)); // Seed random() from floating analog input

  eyes.setup();
}

const SPISettings settings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0);

/// MAIN LOOP -- runs continuously after setup() ----------------------------
void loop() {
  eyes.drawSingleFrame();

  // Switch eyes periodically
  static elapsedMillis eyeTime{};
  if (eyeTime > EYE_DURATION_MS) {
    eyes.next();
    eyeTime = 0;
  }

//  if (lightSensorPin >= 0) {
//    // TODO: implement me!
//    // controller->setPupil(...);
//  }
}