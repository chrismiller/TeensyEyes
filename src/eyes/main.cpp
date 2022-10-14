//This example is configured for using 2 displays on SPI and SPI
//using a ST7789 240x240 display without a CS pin.
//If using a display with a CS pin you can change pin configuration
//in config.h
#define USE_ASYNC_UPDATES

// Define if you wish to see debug information on the ST7789 displays
//#define DEBUG_ST7789

// Define if you wish to debug memory usage.  Only works on T4.x
//#define DEBUG_MEMORY

#define BUTTON_ISR 7
//--------------------------------------------------------------------------
// Uncanny eyes for Adafruit 1.5" OLED (product #1431) or 1.44" TFT LCD
// (#2088).  Works on PJRC Teensy 3.x and on Adafruit M0 and M4 boards
// (Feather, Metro, etc.).  This code uses features specific to these
// boards and WILL NOT work on normal Arduino or other boards!
//
// SEE FILE "config.h" FOR MOST CONFIGURATION (graphics, pins, display type,
// etc).  Probably won't need to edit THIS file unless you're doing some
// extremely custom modifications.
//
// Adafruit invests time and resources providing this open source code,
// please support Adafruit and open-source hardware by purchasing products
// from Adafruit!
//
// Written by Phil Burgess / Paint Your Dragon for Adafruit Industries.
// MIT license.  SPI FIFO insight from Paul Stoffregen's ILI9341_t3 library.
// Inspired by David Boccabella's (Marcwolf) hybrid servo/OLED eye concept.
//--------------------------------------------------------------------------

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <GC9A01A_t3n.h>
#include <array>


typedef struct {        // Struct is defined before including config.h --
  //int8_t  select;       // pin numbers for each eye's screen select line
  int8_t cs;            // Chip select pin.
  int8_t dc;            // DC pin
  int8_t mosi;         // mosi
  int8_t sck;          // sck pin
  int8_t rst;          // reset pin
  int8_t wink;         // and wink button (or -1 if none) specified there,
  uint8_t rotation;     // also display rotation.
  uint8_t init_option;  // option for Init
} eyeInfo_t;

typedef GC9A01A_t3n Display; // Using TFT display(s)

#include "config.h"     // ****** CONFIGURATION IS DONE IN HERE ******

#define RGBColor(r, g, b) GC9A01A_t3n::Color565(r, g, b)

// For autonomous iris scaling
#define  IRIS_LEVELS 7
float iris_prev[IRIS_LEVELS] = {0};
float iris_next[IRIS_LEVELS] = {0};
uint16_t iris_frame = 0;
float irisValue = 0.5f;

#define NUM_EYES (sizeof eyeInfo / sizeof eyeInfo[0]) // config.h pin list

Eye eyes[NUM_EYES]{};
uint8_t eyeIndex{}; // eye[] array counter


OverallState state{};

EyeParams *eyeParams = &defaultEye::params;

float mapToScreen(int value, int mapRadius, int eyeRadius) {
  return sinf((float) value / (float) mapRadius) * M_PI_2 * eyeRadius;
}


// INITIALIZATION -- runs once at startup ----------------------------------

void setup(void) {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  delay(500);
  Serial.println("Init");
  Serial.flush();
  randomSeed(analogRead(A3)); // Seed random() from floating analog input

#ifdef DISPLAY_BACKLIGHT
  // Enable backlight pin, initially off
  Serial.println("Backlight off");
  pinMode(DISPLAY_BACKLIGHT, OUTPUT);
  digitalWrite(DISPLAY_BACKLIGHT, LOW);
#endif


  // Start with the eyes looking straight ahead
  state.eyeOldX = state.eyeNewX = state.eyeOldY = state.eyeNewY = eyeParams->polar.mapRadius;

  // Initialize eye objects based on eyeInfo list in config.h:
  for (uint e = 0; e < NUM_EYES; e++) {
    Serial.print("Create display #");
    Serial.println(e);
    //eye[e].display     = new displayType(&TFT_SPI, eyeInfo[e].cs,
    //                       DISPLAY_DC, -1);
    //for SPI
    //(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
    eyes[e].display = new Display(eyeInfo[e].cs, eyeInfo[e].dc, eyeInfo[e].rst,
                                  eyeInfo[e].mosi, eyeInfo[e].sck);
    eyes[e].blink.state = BlinkState::NotBlinking;
    eyes[e].x = state.eyeOldX;
    eyes[e].y = state.eyeOldY;

    // If project involves only ONE eye and NO other SPI devices, its
    // select line can be permanently tied to GND and corresponding pin
    // in config.h set to -1.  Best to use it though.
    if (eyeInfo[e].cs >= 0) {
      pinMode(eyeInfo[e].cs, OUTPUT);
      digitalWrite(eyeInfo[e].cs, HIGH); // Deselect them all
    }
    // Also set up an individual eye-wink pin if defined:
    if (eyeInfo[e].wink >= 0) pinMode(eyeInfo[e].wink, INPUT_PULLUP);
  }
#if defined(BLINK_PIN) && (BLINK_PIN >= 0)
  pinMode(BLINK_PIN, INPUT_PULLUP); // Ditto for all-eyes blink pin
#endif

#if defined(DISPLAY_RESET) && (DISPLAY_RESET >= 0)
  // Because both displays share a common reset pin, -1 is passed to
  // the display constructor above to prevent the begin() function from
  // resetting both displays after one is initialized.  Instead, handle
  // the reset manually here to take care of both displays just once:
  Serial.println("Reset displays");
  pinMode(DISPLAY_RESET, OUTPUT);
  digitalWrite(DISPLAY_RESET, LOW);  delay(1);
  digitalWrite(DISPLAY_RESET, HIGH); delay(50);
  // Alternately, all display reset pin(s) could be connected to the
  // microcontroller reset, in which case DISPLAY_RESET should be set
  // to -1 or left undefined in config.h.
#endif

  // After all-displays reset, now call init/begin func for each display:
  for (uint e = 0; e < NUM_EYES; e++) {
    eyes[e].display->begin();
    Serial.print("Init ST77xx display #");
    Serial.println(e);
    Serial.println("Rotate");
    eyes[e].display->setRotation(eyeInfo[e].rotation);
  }
  Serial.println("done");

#if defined(LOGO_TOP_WIDTH) || defined(COLOR_LOGO_WIDTH)
  Serial.println("Display logo");
  // I noticed lots of folks getting right/left eyes flipped, or
  // installing upside-down, etc.  Logo split across screens may help:
  for (e = 0; e < NUM_EYES; e++) { // Another pass, after all screen inits
    eye[e].display->fillScreen(0);
#ifdef LOGO_TOP_WIDTH
    // Monochrome Adafruit logo is 2 mono bitmaps:
    eye[e].display->drawBitmap(NUM_EYES * DISPLAY_SIZE / 2 - e * DISPLAY_SIZE - 20,
                               0, logo_top, LOGO_TOP_WIDTH, LOGO_TOP_HEIGHT, 0xFFFF);
    eye[e].display->drawBitmap(NUM_EYES * DISPLAY_SIZE / 2 - e * DISPLAY_SIZE - LOGO_BOTTOM_WIDTH / 2,
                               LOGO_TOP_HEIGHT, logo_bottom, LOGO_BOTTOM_WIDTH, LOGO_BOTTOM_HEIGHT,
                               0xFFFF);
#else
    // Color sponsor logo is one RGB bitmap:
    eye[e].display->fillScreen(color_logo[0]);
    eye[0].display->drawRGBBitmap(
      (eye[e].display->width()  - COLOR_LOGO_WIDTH ) / 2,
      (eye[e].display->height() - COLOR_LOGO_HEIGHT) / 2,
      color_logo, COLOR_LOGO_WIDTH, COLOR_LOGO_HEIGHT);
#endif
    // After logo is drawn
  }
#ifdef DISPLAY_BACKLIGHT
  int i;
  Serial.println("Fade in backlight");
  for (i = 0; i < BACKLIGHT_MAX; i++) { // Fade logo in
    analogWrite(DISPLAY_BACKLIGHT, i);
    delay(2);
  }
  delay(1400); // Pause for screen layout/orientation
  Serial.println("Fade out backlight");
  for (; i >= 0; i--) {
    analogWrite(DISPLAY_BACKLIGHT, i);
    delay(2);
  }
  for (e = 0; e < NUM_EYES; e++) { // Clear display(s)
    eye[e].display->fillScreen(0);
  }
  delay(100);
#else
  delay(2000); // Pause for screen layout/orientation
#endif // DISPLAY_BACKLIGHT
#endif // LOGO_TOP_WIDTH

  // One of the displays is configured to mirror on the X axis.  Simplifies
  // eyelid handling in the drawEye() function -- no need for distinct
  // L-to-R or R-to-L inner loops.  Just the X coordinate of the iris is
  // then reversed when drawing this eye, so they move the same.  Magic!
  // The values for setRotation would be: 0XC8(-MX), 0xA8(+MY), 0x8(-MX), 0x68(+MY)
  // 0xC0, A0, 0, 60
  const uint8_t mirrorTFT[] = {0x8, 0x20, 0x40, 0xE0}; // Mirror+rotate
  eyes[0].display->sendCommand(
      GC9A01A_MADCTL, // Current TFT lib
      &mirrorTFT[eyeInfo[0].rotation & 3], 1);

#ifdef DISPLAY_BACKLIGHT
  Serial.println("Backlight on!");
  analogWrite(DISPLAY_BACKLIGHT, BACKLIGHT_MAX);
#endif
  for (uint e = 0; e < NUM_EYES; e++) {
    if (!eyes[e].display->useFrameBuffer(1)) {
      Serial.printf("%d: Use Frame Buffer failed\n", e);
    } else {
      Serial.printf("$%d: Using Frame buffer\n", e);
    }
  }
}


inline uint8_t upperThreshold(uint8_t x, uint8_t y) {
  const uint8_t start = eyeParams->eyelids.upperOpen(x);
  const uint8_t end = eyeParams->eyelids.upperClosed(x);
  return y <= start ? 0 : y >= end ? 255 : (y - start) * 256 / (end - start);
}

inline uint8_t lowerThreshold(uint8_t x, uint8_t y) {
  const uint8_t start = eyeParams->eyelids.lowerOpen(x);
  const uint8_t end = eyeParams->eyelids.lowerClosed(x);
  return y <= start ? 255 : y >= end ? 0 : (end - y) * 256 / (end - start);
}

/// Compute the Y coordinate of the upper eyelid at a given X coordinate.
/// \param x the X location in pixels.
/// \param proportion the proportion the eyelid is open. 0 = fully closed, 1 = fully open.
/// \return  the Y coordinate in pixels of the edge of the top eyelid.
inline uint8_t upperLid(uint8_t x, float proportion) {
  const uint8_t start = eyeParams->eyelids.upperOpen(x);
  const uint8_t end = eyeParams->eyelids.upperClosed(x);
  return end - proportion * (end - start);
}

/// Compute the Y coordinate of the lower eyelid at a given X coordinate.
/// \param x the X location in pixels.
/// \param proportion the proportion the eyelid is open. 0 = fully closed, 1 = fully open.
/// \return  the Y coordinate in pixels of the edge of the bottom eyelid.
inline uint8_t lowerLid(uint8_t x, float proportion) {
  const uint8_t start = eyeParams->eyelids.lowerClosed(x);
  const uint8_t end = eyeParams->eyelids.lowerOpen(x);
  return start + proportion * (end - start);
}

// EYE-RENDERING FUNCTION --------------------------------------------------

SPISettings settings(SPI_FREQ, MSBFIRST, SPI_MODE0);

// Renders one eye.  Inputs must be pre-clipped & valid.
void drawEye(
    Eye &eye,
    float upperFactor,      // How open the upper eyelid is. 0 = fully closed, 1 = fully open
    float lowerFactor,      // How open the lower eyelid is. 0 = fully closed, 1 = fully open
    float blinkFactor       // How much the eye is blinking. 0 = not blinking, 1 = fully blinking (closed)
    ) {

  const uint8_t displacementMapSize = screenWidth / 2;
  const uint16_t mapRadius = eyeParams->polar.mapRadius;

  Display &display = *eye.display;
  EyeBlink &blink = eye.blink;

  const int xPositionOverMap = eye.x - screenWidth / 2;
  const int yPositionOverMap = eye.y - screenHeight / 2;

  const int iPupilFactor = (int) ((float) eyeParams->iris.texture.height * 256 * (1.0 / irisValue));

  // Dampen the eyelid movement a bit
  upperFactor = eye.upperLidFactor * 0.7f + upperFactor * 0.3f;
  lowerFactor = eye.lowerLidFactor * 0.7f + lowerFactor * 0.3f;

  // Figure out how much the eyelids are open
  const float upperF = upperFactor * (1.0f - blinkFactor);
  const float lowerF = lowerFactor * (1.0f - blinkFactor);
  const float prevUpperF = eye.upperLidFactor * (1.0f - blink.blinkFactor);
  const float prevLowerF = eye.lowerLidFactor * (1.0f - blink.blinkFactor);

  // Store the state for the next iteration
  eye.upperLidFactor = upperFactor;
  eye.lowerLidFactor = lowerFactor;
  blink.blinkFactor = blinkFactor;

  for (uint16_t screenX = 0; screenX < screenWidth; screenX++) {
    // Determine the extents of the eye that need to be drawn, based on where the eyelids
    // are located in both this and the previous frame
    auto currentUpper = upperLid(screenX, upperF);
    auto previousUpper = upperLid(screenX, prevUpperF);
    auto minY = min(currentUpper, previousUpper);
    auto currentLower = lowerLid(screenX, lowerF);
    auto previousLower = lowerLid(screenX, prevLowerF);
    auto maxY = max(currentLower, previousLower);

    // Figure out where we are in the displacement map. The eye (sphere) is symmetrical over
    // X and Y, so we can just swap axes to look up the Y displacement using the same table.
    const uint8_t *displaceX, *displaceY;
    int8_t xmul; // Sign of X displacement: +1 or -1
    int doff; // Offset into displacement arrays
    if (screenX < (screenWidth / 2)) {
      // Left half of screen, so we need to horizontally flip our displacement map lookup
      displaceX = &eyeParams->displacement[displacementMapSize - 1 - screenX];
      displaceY = &eyeParams->displacement[(displacementMapSize - 1 - screenX) * displacementMapSize];
      xmul = -1; // X displacement is always negative
    } else {
      // Right half of screen, so we can lookup horizontally as-is
      displaceX = &eyeParams->displacement[screenX - displacementMapSize];
      displaceY = &eyeParams->displacement[(screenX - displacementMapSize) * displacementMapSize];
      xmul = 1; // X displacement is always positive
    }

    const int xx = xPositionOverMap + screenX;
    for (uint16_t screenY = minY; screenY < maxY; screenY++) {
      uint16_t p;

      if (screenY < currentUpper || screenY >= currentLower) {
        // We're in the eyelid
        p = eyeParams->eyelids.color;
      } else {
        const int yy = yPositionOverMap + screenY;
        int dx, dy;
        if (screenY < displacementMapSize) {
          // We're in the top half of the screen, so we need to vertically flip the displacement map lookup
          doff = displacementMapSize - screenY - 1;
          dy = -displaceY[doff];
        } else {
          // We're in the bottom half of the screen
          doff = screenY - displacementMapSize;
          dy = displaceY[doff];
        }
        dx = displaceX[doff * displacementMapSize];
        if (dx < 255) {
          // We're inside the eyeball (sclera/iris/pupil) area
          dx *= xmul;  // Flip x offset sign if in left half of screen
          int mx = xx + dx;
          int my = yy + dy;

          if (mx >= 0 && mx < mapRadius * 2 && my >= 0 && my < mapRadius * 2) {
            // We're inside the polar angle/distance maps
            uint16_t angle;
            int distance, moff;
            if (my >= mapRadius) {
              my -= mapRadius;
              if (mx >= mapRadius) {
                // Quadrant 1 (bottom right), so we can use the angle/dist lookups directly
                mx -= mapRadius;
                moff = my * mapRadius + mx;
                angle = eyeParams->polar.angle[moff];
                distance = eyeParams->polar.distance[moff];
              } else {
                // Quadrant 2 (bottom left), so rotate angle by 270 degrees clockwise (768) and mirror distance on X axis
                mx = mapRadius - mx - 1;
                angle = eyeParams->polar.angle[mx * mapRadius + my] + 768;
                distance = eyeParams->polar.distance[my * mapRadius + mx];
              }
            } else {
              if (mx < mapRadius) {
                // Quadrant 3 (top left), so rotate angle by 180 degrees and mirror distance on the X and Y axes
                mx = mapRadius - mx - 1;
                my = mapRadius - my - 1;
                moff = my * mapRadius + mx;
                angle = eyeParams->polar.angle[moff] + 512;
                distance = eyeParams->polar.distance[moff];
              } else {
                // Quadrant 4 (top right), so rotate angle by 90 degrees clockwise (256) and mirror distance on Y axis
                mx -= mapRadius;
                my = mapRadius - my - 1;
                angle = eyeParams->polar.angle[mx * mapRadius + my] + 256;
                distance = eyeParams->polar.distance[my * mapRadius + mx];
              }
            }

            // Convert the polar angle/distance to text map coordinates
            if (distance < 128) {
              // We're in the sclera
              // angle = ((angle + currentEye.sclera.angle) & 1023) ^ currentEye.sclera.mirror;
              int tx = (angle & 1023) * eyeParams->sclera.texture.width / 1024; // Texture map x/y
              int ty = distance * eyeParams->sclera.texture.height / 128;
              p = eyeParams->sclera.texture.get(tx, ty);
            } else if (distance < 255) {
              // Either the iris or pupil
              const int ty = (distance - 128) * iPupilFactor / 32768;
              if (ty >= eyeParams->iris.texture.height) {
                // Pupil
                p = eyeParams->pupil.color;
              } else {
                // Iris
                // angle = ((angle + eyes[eyeIndex].iris.angle) & 1023) ^ eyes[eyeIndex].iris.mirror;
                int tx = (angle & 1023) * eyeParams->iris.texture.width / 1024;
                p = eyeParams->iris.texture.get(tx, ty);
              }
            } else {
              // Back of eye
              p = eyeParams->backColor;
            }
          } else {
            // We're outside the polar map so just use the back-of-eye color
            p = eyeParams->backColor;
          }
        } else {
          // We're outside the eye area, i.e. this must be on an eyelid
          p = eyeParams->eyelids.color;
        }
      }
      display.drawPixel(screenX, screenY, p);
    } // end column
  } // end scanline

#ifdef DEBUG_ST7789
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.setTextColor(ST77XX_RED, ST77XX_BLACK);
  display.printf("%4u %4u %5u\n(%3u,%3u)", iScale, irisThreshold, irisScale, scleraX, scleraY);
  display.setCursor(0, DISPLAY_SIZE - 20);
  display.printf("%u %3u %3u %3u %3u\n", uT, lT, blink.state, max_d, max_a);

  // Debug
  static uint32_t iScale_printed[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  bool print_iScale = (iScale_printed[iScale >> 5] & (1 << (iScale & 0x1f))) ? false : true;
  if (print_iScale) {
    iScale_printed[iScale >> 5] |= (1 << (iScale & 0x1f));
    Serial.printf("%4u : %6u %6u %4u:%4u %4u:%4u\n", iScale, irisThreshold, irisScale, min_d, max_d, min_a, max_a);
  }
#endif


  // ==================================================================================
  // FPS counter
  static elapsedMillis ms{};
  static int fps{};
  static int count{};
  count++;
  if (ms >= 1000L) {
    fps = count;
    count = 0;
    ms = 0L;
  }




//  eye.display->setTextSize(2);
//  eye.display->setTextColor(WHITE, BLACK);
//  eye.display->drawNumber(fps, 100, 80);

//  eye.display->drawFloat(upperF, 2, 100, 100);
//  eye.display->drawFloat(lowerF, 2, 100, 120);
//  eye.display->drawFloat(blinkFactor, 2, 100, 140);
  // ==================================================================================


#if defined(USE_ASYNC_UPDATES)
  if (!eye.display->updateScreenAsync()) {
    Serial.println("Eye : updateScreenAsync FAILED");
  } else {
    //Serial.printf("%d : updateScreenAsync started\n", e);
  }
#else
  eye[e].display->updateScreen();
#endif
}

// EYE ANIMATION -----------------------------------------------------------

// Process motion for a single frame of left or right eye
void frame() {
  if (++eyeIndex >= NUM_EYES) {
    // Cycle through eyes, 1 per call
    eyeIndex = 0;
  }
  Eye &eye = eyes[eyeIndex];

#if defined(USE_ASYNC_UPDATES)
  elapsedMillis emWait = 0;
  while (eye.display->asyncUpdateActive() && (emWait < 1000));
  if (emWait >= 1000) Serial.println("Long wait");
#endif

  // Remember the time at start of function
  uint32_t t = micros();

  // X/Y movement

  if (moveEyesRandomly) {
    // Autonomous X/Y eye motion
    // Periodically initiates motion to a new random point, random speed,
    // holds there for random period until next motion.

    // microseconds elapsed since last eye event
    int32_t dt = t - state.moveStartTime;
    if (state.inMotion) {
      // The eye is currently moving
      if (dt >= state.moveDuration) {
        // Time's up, we have reached the destination
        state.inMotion = false;
        // The "move" duration temporarily becomes a hold duration...
        // Normally this is 35 ms to 1 sec, but don't exceed gazeMax setting
        uint32_t limit = min(1000000u, gazeMax);
        state.moveDuration = random(35000u, limit);      // Time between micro-saccades
        if (!state.saccadeInterval) {                    // Cleared when "big" saccade finishes
          state.lastSaccadeStop = t;                    // Time when saccade stopped
          state.saccadeInterval = random(state.moveDuration, gazeMax); // Next in 30ms to 3sec
        }
        // Similarly, the "move" start time becomes the "stop" starting time...
        state.moveStartTime = t;               // Save time of event
        eye.x = state.eyeOldX = state.eyeNewX;           // Save position
        eye.y = state.eyeOldY = state.eyeNewY;
      } else { // Move time's not yet fully elapsed -- interpolate position
        float e = (float) dt / float(state.moveDuration); // 0.0 to 1.0 during move
        float e2 = e * e;
        e = e2 * (3 - 2 * e);     // Easing function: 3*e^2-2*e^3, values in range 0.0 to 1.0
        eye.x = state.eyeOldX + (state.eyeNewX - state.eyeOldX) * e; // Interp X
        eye.y = state.eyeOldY + (state.eyeNewY - state.eyeOldY) * e; // and Y
      }
    } else {
      // Eye is currently stopped
      eye.x = state.eyeOldX;
      eye.y = state.eyeOldY;
      if (dt > state.moveDuration) {
        // It's time to begin a new move
        if ((t - state.lastSaccadeStop) > state.saccadeInterval) {
          // It's time for a 'big' saccade. r is the radius in X and Y that the eye can go, from (0,0) in the center.
          float r = ((float) eyeParams->polar.mapRadius * 2 - (float) screenWidth * M_PI_2) * 0.75f;
          state.eyeNewX = random(-r, r);
          float moveDist = sqrt(r * r - state.eyeNewX * state.eyeNewX);
          state.eyeNewY = random(-moveDist, moveDist);
          // Set the duration for this move, and start it going.
          state.moveDuration = random(83000, 166000); // ~1/12 - ~1/6 sec
          state.saccadeInterval = 0; // Calc next interval when this one stops
        } else {
          // Microsaccade
          // r is possible radius of motion, ~1/10 size of full saccade.
          // We don't bother with clipping because if it strays just a little,
          // that's okay, it'll get put in-bounds on next full saccade.
          float r = (float) eyeParams->polar.mapRadius * 2 - (float) screenWidth * M_PI_2;
          r *= 0.07f;
          float dx = random(-r, r);
          state.eyeNewX = eye.x - eyeParams->polar.mapRadius + dx;
          float h = sqrt(r * r - dx * dx);
          state.eyeNewY = eye.y - eyeParams->polar.mapRadius + random(-h, h);
          state.moveDuration = random(7000, 25000); // 7-25 ms microsaccade
        }
        state.eyeNewX += eyeParams->polar.mapRadius;    // Translate new point into map space
        state.eyeNewY += eyeParams->polar.mapRadius;
        state.moveStartTime = t;    // Save initial time of move
        state.inMotion = true; // Start move on next frame
      }
    }
  } else {
    int eyeTargetX = 0;
    int eyeTargetY = 0;

    // Allow user code to control eye position (e.g. IR sensor, joystick, etc.)
    float r = ((float) eyeParams->polar.mapRadius * 2 - (float) screenWidth * M_PI_2) * 0.9;
    eye.x = eyeParams->polar.mapRadius + eyeTargetX * r;
    eye.y = eyeParams->polar.mapRadius + eyeTargetY * r;
  }

  // Eyes fixate (are slightly crossed)
  const int nufix = 7;
  state.fixate = ((state.fixate * 15) + nufix) / 16;
  eye.x = (eyeIndex & 1) ? eye.x + state.fixate : eye.x - state.fixate;

  float upperQ, lowerQ;
  if (eyeParams->tracking) {
    // Eyelids are set to naturally "track" the pupils (move up or down automatically).
    // Find the pupil position on screen
    uint16_t mapRadius = eyeParams->polar.mapRadius;
    int ix = (int) mapToScreen(mapRadius - eye.x, mapRadius, eyeParams->radius) + screenWidth / 2;
    int iy = (int) mapToScreen(mapRadius - eye.y, mapRadius, eyeParams->radius) + screenWidth / 2;
    iy -= eyeParams->iris.radius * 0.75f; // TODO - the constant is a 'trackFactor', AKA 'squint' in the M4 config
    if (eyeIndex & 1) {
      // Flip for right eye
      ix = screenWidth - 1 - ix;
    }
    uint8_t upperOpen = eyeParams->eyelids.upperOpen(ix);
    if (iy <= upperOpen) {
      upperQ = 1.0f;
    } else {
      uint8_t upperClosed = eyeParams->eyelids.upperClosed(ix);
      if (iy >= upperClosed) {
        upperQ = 0.0f;
      } else {
        upperQ = (float) (upperClosed - iy) / (float) (upperClosed - upperOpen);
      }
    }
    lowerQ = 1.0f - upperQ;
  } else {
    // No tracking - eyelids are fully open unless blink modifies them
    upperQ = 1.0f;
    lowerQ = 1.0f;
  }

  // Handle blinking
  if (autoBlink) {
    // Similar to the autonomous eye movement above -- blink start times
    // and durations are random (within ranges).
    if ((t - state.timeOfLastBlink) >= state.timeToNextBlink) {
      // Start a new blink
      state.timeOfLastBlink = t;
      const uint32_t blinkDuration = random(36000, 72000); // ~1/28 - ~1/14 sec
      // Set up durations for both eyes (if not already winking)
      for (auto &eye: eyes) {
        if (eye.blink.state == BlinkState::NotBlinking) {
          eye.blink.state = BlinkState::BlinkClosing;
          eye.blink.startTime = t;
          eye.blink.duration = blinkDuration;
        }
      }
      state.timeToNextBlink = blinkDuration * 3 + random(4000000);
    }
  }

  auto &blink = eye.blink;
  float blinkFactor{};
  if (blink.state != BlinkState::NotBlinking) {
    // The eye is currently blinking. We scale the upper/lower thresholds relative
    // to the blink position, so that blinks work together with pupil tracking.
    if (t - blink.startTime >= blink.duration) {
      // Advance to the next blink state
      switch (blink.state) {
        case BlinkState::BlinkClosing:
          blink.state = BlinkState::BlinkOpening;
          blink.duration *= 2; // Open the eyelids at half the speed they closed at
          blink.startTime = t;
          blinkFactor = 1.0f;
          break;
        case BlinkState::BlinkOpening:
          blink.state = BlinkState::NotBlinking;
          blinkFactor = 0.0f;
          break;
        default:
          // This should never happen
          break;
      }
    } else {
      blinkFactor = (float) (t - blink.startTime) / (float) blink.duration;
      if (blink.state == BlinkState::BlinkOpening) {
        blinkFactor = 1.0f - blinkFactor;
      }
    }
  }

  // Draw the eye. We temporarily flip the X value if this is the right eye, since it is mirrored
  if (eyeIndex == 0) eye.x = eyeParams->polar.mapRadius * 2 - eye.x;
  drawEye(eye, upperQ, lowerQ, blinkFactor);
  if (eyeIndex == 0) eye.x = eyeParams->polar.mapRadius * 2 - eye.x;
}

// MAIN LOOP -- runs continuously after setup() ----------------------------

void loop() {
  if (autoPupilResize) {
    // Not light responsive. Use autonomous iris w/fractal subdivision
    float n, sum = 0.5f;
    for (uint16_t i = 0; i < IRIS_LEVELS; i++) { // 0,1,2,3,...
      uint16_t iexp = 1 << (i + 1);          // 2,4,8,16,...
      uint16_t imask = (iexp - 1);          // 2^i-1 (1,3,7,15,...)
      uint16_t ibits = iris_frame & imask;  // 0 to mask
      if (ibits) {
        float weight = (float) ibits / (float) iexp; // 0.0 to <1.0
        n = iris_prev[i] * (1.0f - weight) + iris_next[i] * weight;
      } else {
        n = iris_next[i];
        iris_prev[i] = iris_next[i];
        iris_next[i] = -0.5f + ((float) random(1000) / 999.0f); // -0.5 to +0.5
      }
      iexp = 1 << (IRIS_LEVELS - i); // ...8,4,2,1
      sum += n / (float) iexp;
    }

    const float irisMin = 1.0f - eyeParams->pupil.max;
    const float irisRange = eyeParams->pupil.max - eyeParams->pupil.min;
    irisValue = irisMin + sum * irisRange;

    if ((++iris_frame) >= (1 << IRIS_LEVELS)) {
      iris_frame = 0;
    }
  } else if (lightSensorPin >= 0) {
    // TODO: implement me!
  }

  frame();
}

// from the linker
//  extern unsigned long _stextload;
extern unsigned long _stext;
extern unsigned long _etext;
//  extern unsigned long _sdataload;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;
//  extern unsigned long _flexram_bank_config;
extern unsigned long _estack;

void DumpMemoryInfo() {
#if defined(__IMXRT1062__) && defined(DEBUG_MEMORY)
  uint32_t flexram_config = IOMUXC_GPR_GPR17;
  Serial.printf("IOMUXC_GPR_GPR17:%x IOMUXC_GPR_GPR16:%x IOMUXC_GPR_GPR14:%x\n",
                flexram_config, IOMUXC_GPR_GPR16, IOMUXC_GPR_GPR14);
  Serial.printf("Initial Stack pointer: %x\n", &_estack);
  uint32_t dtcm_size = 0;
  uint32_t itcm_size = 0;
  for (; flexram_config; flexram_config >>= 2) {
    if ((flexram_config & 0x3) == 0x2) dtcm_size += 32768;
    else if ((flexram_config & 0x3) == 0x3) itcm_size += 32768;
  }
  Serial.printf("ITCM allocated: %u  DTCM allocated: %u\n", itcm_size, dtcm_size);
  Serial.printf("ITCM init range: %x - %x Count: %u\n", &_stext, &_etext, (uint32_t)&_etext - (uint32_t)&_stext);
  Serial.printf("DTCM init range: %x - %x Count: %u\n", &_sdata, &_edata, (uint32_t)&_edata - (uint32_t)&_sdata);
  Serial.printf("DTCM cleared range: %x - %x Count: %u\n", &_sbss, &_ebss, (uint32_t)&_ebss - (uint32_t)&_sbss);
  Serial.printf("Now fill rest of DTCM with known pattern(%x - %x\n", (&_ebss + 1), (&itcm_size - 10)); Serial.flush(); //
  // Guess of where it is safe to fill memory... Maybe address of last variable we have defined - some slop...
  for (uint32_t *pfill = (&_ebss + 32); pfill < (&itcm_size - 10); pfill++) {
    *pfill = 0x01020304;  // some random value
  }
#endif
}

void EstimateStackUsage() {
#if defined(__IMXRT1062__) && defined(DEBUG_MEMORY)
  uint32_t *pmem = (&_ebss + 32);
  while (*pmem == 0x01020304) pmem++;
  Serial.printf("Estimated max stack usage: %d\n", (uint32_t)&_estack - (uint32_t)pmem);
#endif
}