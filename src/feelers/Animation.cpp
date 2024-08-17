#include "Animation.h"

struct MovementDefinition {
  Movement *left;
  Movement *right;
  uint32_t duration;
};

std::vector<MovementDefinition> movements{
    MovementDefinition{
        new StopMovement(1000),
        new StopMovement(1000),
        2'000
    },
    MovementDefinition{
        new CircularMovement(3000),
        new CircularMovement(3000, true),
        20'000
    },
    MovementDefinition{
        new DiagonalMovement(2000),
        new DiagonalMovement(2000, true),
        20'000
    },
    MovementDefinition{
        new SidewaysMovement(1000),
        new SidewaysMovement(1000),
        10'000
    },
    MovementDefinition{
        new RandomMovement(),
        new RandomMovement(),
        30'000
    },
    MovementDefinition{
        new VerticalMovement(2500),
        new VerticalMovement(2500),
        20'000
    },
    MovementDefinition{
        new StopMovement(4000),
        new StopMovement(4000),
        10'000
    },
    MovementDefinition{
        new RandomMovement(),
        new RandomMovement(),
        45'000
    },
    MovementDefinition{
        new CircularMovement(3000),
        new CircularMovement(3000, true),
        30'000
    },
    MovementDefinition{
        new SidewaysMovement(1000),
        new SidewaysMovement(1000, true),
        12'000
    },
    MovementDefinition{
        new DiagonalMovement(2000),
        new DiagonalMovement(2000),
        15'000
    },
    MovementDefinition{
        new RandomMovement(),
        new RandomMovement(),
        40'000
    },
    MovementDefinition{
        new StopMovement(6000),
        new StopMovement(6000),
        10'000
    },
    MovementDefinition{
        new RandomMovement(),
        new RandomMovement(),
        10'000
    }
};

void Animation::init(int leftLR, int leftUD, int rightLR, int rightUD) {
  enabled = leftLR >= 0 && leftUD >= 0 && rightLR >= 0 && rightUD >= 0;
  if (!enabled) {
    Serial.print("No feelers configured");
    return;
  }
  Serial.println("Feelers configured");
  leftFeeler = new Feeler();
  rightFeeler = new Feeler();
  leftFeeler->init(leftLR, leftUD, movements[0].left);
  rightFeeler->init(rightLR, rightUD, movements[0].right);
  last_movement_change = millis();
}

void Animation::update() {
  if (!enabled) {
    return;
  }
  uint32_t now = millis();
  if ((now - last_update) < UPDATE_MS) {
    return;
  }
  last_update = now;
  if (now - last_movement_change > movements[movementIndex].duration) {
    // It's time to switch to the next movement animation
    last_movement_change = now;
    movementIndex = (movementIndex + 1) % movements.size();
    MovementDefinition md = movements[movementIndex];
    leftFeeler->setMovement(md.left);
    rightFeeler->setMovement(md.right);
  }
  leftFeeler->update();
  rightFeeler->update();
}