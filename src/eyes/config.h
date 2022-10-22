//#define SERIAL_tt Serial // Send debug_tt output here. Must have SERIAL_tt.begin( ## )
//#include "debug_tt.h"
// Pin selections here are based on the original Adafruit Learning System
// guide for the Teensy 3.x project.  Some of these pin numbers don't even
// exist on the smaller SAMD M0 & M4 boards, so you may need to make other
// selections:

// GRAPHICS SETTINGS (appearance of eye) -----------------------------------

// If using a SINGLE EYE, you might want this next line enabled, which
// uses a simpler "football-shaped" eye that's left/right symmetrical.
// Default shape includes the caruncle, creating distinct left/right eyes.
// Otherwise your choice, standard is asymmetrical
//#define SYMMETRICAL_EYELID

#include "graphics/eyes.h"

// Enable the eye(s) you want to #include -- HUGE graphics tables for various eyes:
#ifdef LARGE_EYES
#include "eyes/graphics/240x240/anime.h"
#include "eyes/graphics/240x240/bigBlue.h"
#include "eyes/graphics/240x240/demon.h"
#include "eyes/graphics/240x240/doomRed.h"
#include "eyes/graphics/240x240/doomSpiral.h"
#include "eyes/graphics/240x240/fish.h"
#include "eyes/graphics/240x240/fizzgig.h"
#include "eyes/graphics/240x240/hazel.h"        // Standard human-ish hazel eye
#include "eyes/graphics/240x240/hypnoRed.h"
#include "eyes/graphics/240x240/skull.h"
#include "eyes/graphics/240x240/snake.h"
//#include "eyes/graphics/240x240/spikes.h"
//#include "eyes/graphics/240x240/toonstripe.h"
#else
#include "graphics/128x128/defaultEye.h"      // Standard human-ish hazel eye -OR-
//#include "graphics/128x128/dragonEye.h"     // Slit pupil fiery dragon/demon eye -OR-
//#include "graphics/128x128/noScleraEye.h"   // Large iris, no sclera -OR-
//#include "graphics/128x128/goatEye.h"       // Horizontal pupil goat/Krampus eye -OR-
//#include "graphics/128x128/newtEye.h"       // Eye of newt -OR-
//#include "graphics/128x128/terminatorEye.h" // Git to da choppah!
//#include "graphics/128x128/catEye.h"        // Cartoonish cat (flat "2D" colors)
//#include "graphics/128x128/owlEye.h"        // Minerva the owl (DISABLE TRACKING)
//#include "graphics/128x128/naugaEye.h"      // Nauga googly eye (DISABLE TRACKING)
//#include "graphics/128x128/doeEye.h"        // Cartoon deer eye (DISABLE TRACKING)
//#include "graphics/128x128/Nebula.h"  //Dont work
//#include "graphics/128x128/MyEyeHuman1.h"
//#include "graphics/128x128/Human-HAL9000.h"
//#include "graphics/128x128/NebulaBlueGreen.h"
//#include "graphics/128x128/SpiralGalaxy.h"
//#include "graphics/128x128/ChameleonX_Eye.h"  //no work
//#include "graphics/128x128/MyEye.h"
#endif
// Optional: enable this line for startup logo (screen test/orient):

// EYE LIST ----------------------------------------------------------------


// DISPLAY HARDWARE SETTINGS (screen type & connections) -------------------

  //#define TFT_SPI        SPI
  //#define TFT_PERIPH     PERIPH_SPI

  // Enable ONE of these #includes to specify the display type being used
#include <GC9A01A_t3n.h>
  
  #define SPI_FREQ 48000000    // TFT: use max SPI (clips to 12 MHz on M0)

// This table contains ONE LINE PER EYE.  The table MUST be present with
// this name and contain ONE OR MORE lines.  Each line contains THREE items:
// a pin number for the corresponding TFT/OLED display's SELECT line, a pin
// pin number for that eye's "wink" button (or -1 if not used), and a screen
// rotation value (0-3) for that eye.

eyeInfo_t eyeInfo[] = {
  //CS  DC MOSI SCK RST WINK ROT INIT
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
   {29,31,  21, 32,  28, -1,  0, 0 }, // RIGHT EYE display-select and wink pins, no rotation
   {10, 9,  11, 13,  8, -1,  0, 0 }, // LEFT EYE display-select and wink pins, no rotation
#else
   {0,  2,  26, 27,  3, -1,  0, 0 }, // RIGHT EYE display-select and wink pins, no rotation
   {10, 9,  11, 13,  8, -1,  0, 0 }, // LEFT EYE display-select and wink pins, no rotation
#endif
};


// INPUT SETTINGS (for controlling eye motion) -----------------------------

bool autoBlink{true};         // If enabled, the eyes blink autonomously
bool moveEyesRandomly{true};  // If enabled, the eyes look around randomly

bool autoPupilResize{true};   // If enabled, the pupils resize autonomously
int8_t lightSensorPin{-1};    // Otherwise, set this pin to use a light sensor
uint16_t  lightSensorMin{0};
uint16_t  lightSensorMax{1023};

int8_t blinkPin{-1};          // A button to make the eyes blink

uint32_t gazeMax{3000000};    // Max wait time (microseconds) for major eye movements