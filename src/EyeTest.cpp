#include <cstdint>
#include <array>

#include "EyeTest.h"
#include "config.h"

// The index of the currently selected eye definitions
static uint32_t defIndex{0};

std::array<std::array<EyeDefinition, 2>, 13> eyeDefinitions{{
                                                                {anime::eye, anime::eye},
//                                                                {bigBlue::eye, bigBlue::eye},
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

// INITIALIZATION -- runs once at startup ----------------------------------


void EyeTest::setup() {
  auto &defs = eyeDefinitions.at(defIndex);
  auto l = new GC9A01A_Display(eyeInfo[0]);
  auto r = new GC9A01A_Display(eyeInfo[1]);
  const DisplayDefinition<GC9A01A_Display> left{l, defs[0]};
  const DisplayDefinition<GC9A01A_Display> right{r, defs[1]};
  controller = new EyeController<2, GC9A01A_Display>({left, right}, true, true, true);
}

// EYE-RENDERING FUNCTION --------------------------------------------------

void EyeTest::next() const {
  defIndex = (defIndex + 1) % eyeDefinitions.size();
  controller->updateDefinitions(eyeDefinitions.at(defIndex));
}

void EyeTest::previous() const {
  defIndex = (defIndex + eyeDefinitions.size() - 1) % eyeDefinitions.size();
  controller->updateDefinitions(eyeDefinitions.at(defIndex));
}

void EyeTest::blink() const {
  controller->blink();
}

void EyeTest::autoMove() const {
  controller->setAutoMove(!controller->autoMoveEnabled());
}

void EyeTest::autoPupils() const {
  controller->setAutoPupils(!controller->autoPupilsEnabled());
}

void EyeTest::wink(size_t eyeIndex) const {
  controller->wink(eyeIndex);
}

void EyeTest::setMaxGazeMs(uint32_t maxGazeMs) {
  controller->setMaxGazeMs(maxGazeMs);
  longGaze = !longGaze;
}

// MAIN LOOP -- runs continuously after setup() ----------------------------

void EyeTest::drawSingleFrame() {
  controller->renderFrame();
}
