#pragma once

#include <Arduino.h>
#include <Servo.h>
#include "Movement.h"

class Feeler {
private:
    static const int UPDATE_RATE_MS{20};

    Servo lrServo;
    Servo udServo;
    uint32_t lastUpdateTimeMs{};
    uint32_t moveStartTimeMs{};
    Target target{};
    Movement *movement{};
    // X/Y coords are in the range [-1, 1]
    float currentX{};
    float currentY{};
    float startX{};
    float startY{};

public:
    Feeler();

    /**
     * Initialise the feeler servos.
     * @param lrPin The data pin that's connected to the left-right servo.
     * @param udPin The data pin that's connected to the up-down servo.
     */
    void init(int lrPin, int udPin, Movement *pMovement);

    /**
     * Sets the type of movement for this feeler.
     * @param m
     */
    void setMovement(Movement *m);

    /**
     * Update the servo positions with random feeler motion.
     */
    void update();
};