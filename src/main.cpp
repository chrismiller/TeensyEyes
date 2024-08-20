// Define if you wish to debug memory usage.  Only works on T4.x
//#define DEBUG_MEMORY

#include <SPI.h>
#include <array>
#include <Wire.h>
#include <Entropy.h>

#include "config.h"
#include "util/logging.h"
#include "sensors/LightSensor.h"
#include "sensors/PersonSensor.h"
#include "feelers/Animation.h"

// The index of the currently selected eye definitions
static uint32_t defIndex{0};

TwoWire WIRE = Wire2;
LightSensor lightSensor(LIGHT_PIN);
PersonSensor personSensor(WIRE);
bool personSensorFound = USE_PERSON_SENSOR;
Animation feelerAnimation{};

bool hasBlinkButton() {
  return BLINK_PIN >= 0;
}

bool hasLightSensor() {
  return LIGHT_PIN >= 0;
}

bool hasJoystick() {
  return JOYSTICK_X_PIN >= 0 && JOYSTICK_Y_PIN >= 0;
}

bool hasPersonSensor() {
  return personSensorFound;
}

/// INITIALIZATION -- runs once at startup ----------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  delay(200);
  DumpMemoryInfo();
  Serial.println("Init");
  Serial.flush();
  Entropy.Initialize();
  randomSeed(Entropy.random());

  if (hasBlinkButton()) {
    pinMode(BLINK_PIN, INPUT_PULLUP);
  }

  if (hasJoystick()) {
    pinMode(JOYSTICK_X_PIN, INPUT);
    pinMode(JOYSTICK_Y_PIN, INPUT);
  }

  if (hasPersonSensor()) {
    WIRE.begin();
    personSensorFound = personSensor.isPresent();
    if (personSensorFound) {
      Serial.println("Person Sensor detected");
      personSensor.enableID(false);
      // personSensor.enableLED(false);
      personSensor.setMode(PersonSensor::Mode::Continuous);
    } else {
      Serial.println("No Person Sensor was found!");
    }
  }

  initEyes(!hasJoystick(), !hasBlinkButton(), !hasLightSensor());

  feelerAnimation.init(LEFT_FEELER_LR_PIN, LEFT_FEELER_UD_PIN, RIGHT_FEELER_LR_PIN, RIGHT_FEELER_UD_PIN, FEELER_BUTTON_PIN);
}

void nextEye() {
  defIndex = (defIndex + 1) % eyeDefinitions.size();
  eyes->updateDefinitions(eyeDefinitions.at(defIndex));
}

/// MAIN LOOP -- runs continuously after setup() ----------------------------
void loop() {
  // Update the feeler positions
  feelerAnimation.update();

  // Switch eyes periodically
  static elapsedMillis eyeTime{};
  static boolean evenIteration{};
  if ((evenIteration && (eyeTime > EYE_DURATION_MS / 15)) || (!evenIteration && (eyeTime > EYE_DURATION_MS))) {
    nextEye();
    Serial.print("Changing to eye ");
    Serial.println(defIndex);
    eyeTime = 0;
    if (defIndex == 0) {
      // We've been through all the eyes, adjust the timing so we cycle through them faster/slower this time around
      evenIteration = !evenIteration;
    }
  }

  // Blink on button press
  if (hasBlinkButton() && digitalRead(BLINK_PIN) == LOW) {
    eyes->blink();
  }

  // Move eyes with an analog joystick
  if (hasJoystick()) {
    auto x = analogRead(JOYSTICK_X_PIN);
    auto y = analogRead(JOYSTICK_Y_PIN);
    eyes->setPosition((x - 512) / 512.0f, (y - 512) / 512.0f);
  }

  if (hasLightSensor()) {
    lightSensor.readDamped([](float value) {
      eyes->setPupil(value);
    });
  }

  static float prevTargetX{64.0};
  static float prevTargetY{64.0};
  if (hasPersonSensor() && personSensor.read()) {
    // Find the closest face that is facing the camera, if any
    int maxSize = 0;
    person_sensor_face_t maxFace{};
    
    for (int i = 0; i < personSensor.numFacesFound(); i++) {
      const person_sensor_face_t face = personSensor.faceDetails(i);
      if (face.is_facing && face.box_confidence > 60) {
        int size = (face.box_right - face.box_left) * (face.box_bottom - face.box_top);
        if (size > maxSize) {
          maxSize = size;
          maxFace = face;
        }
      }
    }

    if (maxSize > 0) {
      eyes->setAutoMove(false);
      float targetX = -((static_cast<float>(maxFace.box_left) + static_cast<float>(maxFace.box_right - maxFace.box_left) / 2.0f) / 127.5f - 1.0f);
      float targetY = (static_cast<float>(maxFace.box_top) + static_cast<float>(maxFace.box_bottom - maxFace.box_top) / 3.0f) / 127.5f - 1.0f;
      // Take the average of this and the previous centre, to stop any unwanted movement due to the PersonSensor's
      // bounding box sometimes jittering across alternate frames
      float averageX = prevTargetX != 64.0f ? (targetX + prevTargetX) / 2.0f : targetX;
      float averageY = prevTargetY != 64.0f ? (targetY + prevTargetY) / 2.0f : targetY;
      prevTargetX = targetX;
      prevTargetY = targetY;
      eyes->setTargetPosition(averageX, averageY, 100);
    } else if (personSensor.timeSinceFaceDetectedMs() > 2'000 && !eyes->autoMoveEnabled()) {
      // We haven't seen a face for a while so enable automove
      eyes->setAutoMove(true);
      prevTargetX = 64.0f;
      prevTargetY = 64.0f;
    }
  }

  eyes->renderFrame();
}
