// Define if you wish to debug memory usage.  Only works on T4.x
//#define DEBUG_MEMORY

#include <SPI.h>
#include <array>

#include "config.h"
#include "util/logging.h"
#include "eyes/EyeController.h"
#include "displays/GC9A01A_Display.h"

constexpr uint32_t EYE_DURATION_MS{4'000};

// TFT: use max SPI (clips to 12 MHz on M0)
constexpr uint32_t SPI_FREQUENCY{48'000'000};
const SPISettings settings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0);

// A list of all the different eye definitons we want to use
std::array<std::array<EyeDefinition, 2>, 13> eyeDefinitions{{
//                                                                {anime::eye, anime::eye},
                                                                {bigBlue::eye, bigBlue::eye},
                                                                {brown::eye, brown::eye},
                                                                {cat::eye, cat::eye},
                                                                {demon::left, demon::right},
                                                                {doe::left, doe::right},
                                                                {doomRed::eye, doomRed::eye},
                                                                {doomSpiral::left, doomSpiral::right},
                                                                {dragon::eye, dragon::eye},
//                                                                {fish::eye, fish::eye},
                                                                {fizzgig::eye, fizzgig::eye},
//                                                                {hazel::eye, hazel::eye},
                                                                {hypnoRed::eye, hypnoRed::eye},
//                                                                {newt::eye, newt::eye},
                                                                {skull::eye, skull::eye},
                                                                {snake::eye, snake::eye},
//                                                                {spikes::eye, spikes::eye}
                                                                {toonstripe::eye, toonstripe::eye},
                                                            }
};

// The index of the currently selected eye definitions
static uint32_t defIndex{0};

EyeController<2, GC9A01A_Display> *controller{};

/// INITIALIZATION -- runs once at startup ----------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  delay(200);
  DumpMemoryInfo();
  Serial.println("Init");
  Serial.flush();
  randomSeed(analogRead(A3)); // Seed random() from floating analog input

  // Create the displays and eye controller
  auto &defs = eyeDefinitions.at(defIndex);
  auto l = new GC9A01A_Display(eyeInfo[0]);
  auto r = new GC9A01A_Display(eyeInfo[1]);
  const DisplayDefinition<GC9A01A_Display> left{l, defs[0]};
  const DisplayDefinition<GC9A01A_Display> right{r, defs[1]};
  controller = new EyeController<2, GC9A01A_Display>({left, right}, true, true, true);
}

void nextEye() {
  defIndex = (defIndex + 1) % eyeDefinitions.size();
  controller->updateDefinitions(eyeDefinitions.at(defIndex));
}

/// MAIN LOOP -- runs continuously after setup() ----------------------------
void loop() {
  controller->renderFrame();

  // Switch eyes periodically
  static elapsedMillis eyeTime{};
  if (eyeTime > EYE_DURATION_MS) {
    nextEye();
    eyeTime = 0;
  }
}