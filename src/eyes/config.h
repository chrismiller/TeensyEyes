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
#if(DISPLAY_SIZE == 240)
#include "eyes/graphics/240x240/defaultEye.h"    // Standard human-ish hazel eye
//#include "eyes/graphics/240x240/doeEye.h"        // Cartoon deer eye (DISABLE TRACKING)
#include "eyes/graphics/240x240/newtEye.h"       // Eye of newt
//#include "eyes/graphics/240x240/terminatorEye.h" // Git to da choppah!

//#include "eyes/graphics/240x240/catEye.h"        // Cartoonish cat (flat "2D" colors)
//#include "eyes/graphics/240x240/dragonEye.h"     // Slit pupil fiery dragon/demon eye
//#include "eyes/graphics/240x240/goatEye.h"       // Horizontal pupil goat/Krampus eye
//#include "eyes/graphics/240x240/naugaEye.h"      // Nauga googly eye (DISABLE TRACKING)
//#include "eyes/graphics/240x240/owlEye.h"        // Minerva the owl (DISABLE TRACKING)
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

// JOYSTICK_X_PIN and JOYSTICK_Y_PIN specify analog input pins for manually
// controlling the eye with an analog joystick.  If set to -1 or if not
// defined, the eye will move on its own.
// IRIS_PIN speficies an analog input pin for a photocell to make pupils
// react to light (or potentiometer for manual control).  If set to -1 or
// if not defined, the pupils will change on their own.
// BLINK_PIN specifies an input pin for a button (to ground) that will
// make any/all eyes blink.  If set to -1 or if not defined, the eyes will
// only blink if AUTOBLINK is defined, or if the eyeInfo[] table above
// includes wink button settings for each eye.

//#define JOYSTICK_X_PIN A3 // Analog pin for eye horiz pos (else auto)
//#define JOYSTICK_Y_PIN A4 // Analog pin for eye vert position (")
//#define JOYSTICK_X_FLIP   // If defined, reverse stick X axis
//#define JOYSTICK_Y_FLIP   // If defined, reverse stick Y axis


#define TRACKING            // If defined, eyelid tracks pupil
#define AUTOBLINK           // If defined, eyes also blink autonomously
//#define BLINK_PIN         16 // Pin for manual blink button (BOTH eyes)
  //#define LIGHT_PIN      A3 // Photocell or potentiometer (else auto iris)
  //#define LIGHT_PIN_FLIP    // If defined, reverse reading from dial/photocell
  #define LIGHT_MIN       0 // Lower reading from sensor
  #define LIGHT_MAX    1023 // Upper reading from sensor

#define IRIS_SMOOTH         // If enabled, filter input from IRIS_PIN