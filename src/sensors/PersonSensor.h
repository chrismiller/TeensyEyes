#pragma once

#include <Arduino.h>

// The person sensor will never output more than four faces.
#define PERSON_SENSOR_MAX_FACES_COUNT (4)

// How many different faces the sensor can recognize.
#define PERSON_SENSOR_MAX_IDS_COUNT (7)

// The following structures represent the data format returned from the person
// sensor over the I2C communication protocol. The C standard doesn't
// guarantee the byte-wise layout of a regular struct across different
// platforms, so we add the non-standard (but widely supported) __packed__
// attribute to ensure the layouts are the same as the wire representation.

// The results returned from the sensor have a short header providing
// information about the length of the data packet:
//   reserved: Currently unused bytes.
//   data_size: Length of the entire packet, excluding the header and checksum.
//     For version 1.0 of the sensor, this should be 40.
typedef struct __attribute__ ((__packed__)) {
  uint8_t reserved[2];  // Bytes 0-1.
  uint16_t data_size;   // Bytes 2-3.
} person_sensor_results_header_t;

// Each face found has a set of information associated with it:
//   box_confidence: How certain we are we have found a face, from 0 to 255.
//   box_left: X coordinate of the left side of the box, from 0 to 255.
//   box_top: Y coordinate of the top edge of the box, from 0 to 255.
//   box_width: Width of the box, where 255 is the full view port size.
//   box_height: Height of the box, where 255 is the full view port size.
//   id_confidence: How sure the sensor is about the recognition result.
//   id: Numerical ID assigned to this face.
//   is_looking_at: Whether the person is facing the camera, 0 or 1.
typedef struct __attribute__ ((__packed__)) {
  uint8_t box_confidence;   // Byte 1.
  uint8_t box_left;         // Byte 2.
  uint8_t box_top;          // Byte 3.
  uint8_t box_right;        // Byte 4.
  uint8_t box_bottom;       // Byte 5.
  int8_t id_confidence;     // Byte 6.
  int8_t id;                // Byte 7
  uint8_t is_facing;        // Byte 8.
} person_sensor_face_t;

// This is the full structure of the packet returned over the wire from the
// sensor when we do an I2C read from the peripheral address.
// The checksum should be the CRC16 of bytes 0 to 38. You shouldn't need to
// verify this in practice, but we found it useful during our own debugging.
typedef struct __attribute__ ((__packed__)) {
  person_sensor_results_header_t header;                     // Bytes 0-4.
  int8_t num_faces;                                          // Byte 5.
  person_sensor_face_t faces[PERSON_SENSOR_MAX_FACES_COUNT]; // Bytes 6-37.
  uint16_t checksum;                                         // Bytes 38-39.
} person_sensor_results_t;

class PersonSensor {
private:
  enum class Reg {
    Mode = 0x01, EnableID = 0x02, SingleShot = 0x03, CalibrateID = 0x04, PersistIDs = 0x05,
    EraseIDs = 0x06, DebugMode = 0x07
  };

  static constexpr long SAMPLE_TIME_MS = 200;

  person_sensor_results_t results{};

  elapsedMillis timeSinceSampledMs{SAMPLE_TIME_MS};
  elapsedMillis lastDetectionTimeMs{};

  static void writeReg(Reg reg, uint8_t value);

public:
  enum class Mode {
    Standby = 0x00, Continuous = 0x01
  };

  /// Fetch the latest results from the sensor. Returns false if the read didn't succeed.
  bool read();

  /**
   * Sets the mode to either continuous or standby. In standby mode, face detection must be triggered manually
   * by calling singleShot().
   */
  void setMode(Mode mode) {
    writeReg(Reg::Mode, static_cast<uint8_t>(mode));
  }

  /**
   * Enables or disables the face recognition.
   */
  void enableID(bool enabled) {
    writeReg(Reg::EnableID, enabled);
  }

  /**
   * Performs a single face detection. This is only valid to call in standby mode.
   */
  void singleShot() {
    writeReg(Reg::SingleShot, 0);
  }

  /**
   * Calibrate the next identified frame as person N, from 0 to 7. If two frames pass with no person,
   * this label is discarded.
   */
  void labelNextID() {
    writeReg(Reg::CalibrateID, 0);
  }

  /**
   * Whether to store any recognized IDs even when power is switched off.
   */
  void persistIDs(bool enabled) {
    writeReg(Reg::PersistIDs, enabled);
  }

  /**
   * Wipe any recognized IDs from storage.
   */
  void eraseIDs() {
    writeReg(Reg::EraseIDs, 0);
  }

  /**
   * Whether to enable the LED indicator on the sensor.
   */
  void enableLED(bool enabled) {
    writeReg(Reg::DebugMode, enabled);
  }

  unsigned long timeSinceFaceDetectedMs() {
    return static_cast<long>(lastDetectionTimeMs);
  }
  /**
   * @return the number of faces that were detected at the last successful reading.
   */
  int numFacesFound() const {
    return results.num_faces;
  }

  /**
   * Returns face detection results.
   *
   * @param faceNumber the face number to return detection results for. If this is greater or equal
   * to the number of faces detected, a zeroed out data structure will be returned.
   * @return the details (confidence level and bounding box) of a detected face.
   */
  person_sensor_face_t faceDetails(int faceNumber) {
    if (faceNumber >= results.num_faces) {
      return person_sensor_face_t{};
    }
    return results.faces[faceNumber];
  }
};