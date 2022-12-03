#pragma once

#include "eyes/eyes.h"

// Enable the eye(s) you want to #include -- these are large graphics tables for various eyes:
//#include "eyes/240x240/anime.h"
#include "eyes/240x240/bigBlue.h"
#include "eyes/240x240/brown.h"
#include "eyes/240x240/cat.h"
#include "eyes/240x240/demon.h"
#include "eyes/240x240/doe.h"
#include "eyes/240x240/doomRed.h"
#include "eyes/240x240/doomSpiral.h"
#include "eyes/240x240/dragon.h"
//#include "eyes/240x240/fish.h"
#include "eyes/240x240/fizzgig.h"
//#include "eyes/240x240/hazel.h"
#include "eyes/240x240/hypnoRed.h"
//#include "eyes/240x240/newt.h"
#include "eyes/240x240/skull.h"
#include "eyes/240x240/snake.h"
//#include "eyes/240x240/spikes.h"
#include "eyes/240x240/toonstripe.h"

#include "displays/GC9A01A_Display.h"

// A list of all the different eye definitions we want to use
std::array<std::array<EyeDefinition, 2>, 13> eyeDefinitions{{
//                                                               {anime::eye, anime::eye},
                                                               {bigBlue::eye, bigBlue::eye},
                                                               {brown::eye, brown::eye},
                                                               {cat::eye, cat::eye},
                                                               {demon::left, demon::right},
                                                               {doe::left, doe::right},
                                                               {doomRed::eye, doomRed::eye},
                                                               {doomSpiral::left, doomSpiral::right},
                                                               {dragon::eye, dragon::eye},
//                                                               {fish::eye, fish::eye},
                                                               {fizzgig::eye, fizzgig::eye},
//                                                               {hazel::eye, hazel::eye},
                                                               {hypnoRed::eye, hypnoRed::eye},
//                                                               {newt::eye, newt::eye},
                                                               {skull::eye, skull::eye},
                                                               {snake::eye, snake::eye},
//                                                                {spikes::eye, spikes::eye}
                                                               {toonstripe::eye, toonstripe::eye},
                                                           }
};

// DISPLAY HARDWARE SETTINGS (screen type & connections) -------------------

// Define the pins used for each display. If multiple displays use the same reset pin,
// only specify it on the FIRST display and leave the other set to -1. This prevents the
// GC9A01A_t3n->begin() function from resetting both displays after one is initialized.
// With two displays, one should be configured to mirror on the X axis. This simplifies
// eyelid handling -- no need for distinct L-to-R or R-to-L inner loops. Just the X
// coordinate of the iris is then reversed when drawing this eye, so they move the same.
GC9A01A_Config eyeInfo[] = {
    // CS DC MOSI SCK RST ROT MIRROR USE_FB ASYNC
    {0,  2, 26, 27, 3, 0, true,  true, true}, // Left display
    {10, 9, 11, 13, 8, 0, false, true, true}, // Right display
};

constexpr uint32_t EYE_DURATION_MS{4'000};

/// The speed of the SPI bus. For maximum performance, set this as high as you can get away with.
/// It will depend on the displays themselves, wire lengths, shielding/interference etc. My
/// setup works up to about 90,000,000. At 100,000,000 I start seeing corruption on the displays.
constexpr uint32_t SPI_SPEED{30'000'000};

// Set to -1 to disable the blink button and/or joystick
constexpr int8_t BLINK_PIN{-1};
constexpr int8_t JOYSTICK_X_PIN{-1};
constexpr int8_t JOYSTICK_Y_PIN{-1};
constexpr int8_t LIGHT_PIN{-1};