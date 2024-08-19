#include "Animation.h"

static const std::vector<MovementDefinition> movements{
    MovementDefinition{
        new StopMovement(1000),
        new StopMovement(1000),
        1'000
    },
    MovementDefinition{
        new CircularMovement(4000, true),
        new CircularMovement(4000),
        10'000
    },
    MovementDefinition{
        new DiagonalMovement(2500),
        new DiagonalMovement(2500, true),
        10'000
    },
    MovementDefinition{
        new SidewaysMovement(2000),
        new SidewaysMovement(2000),
        10'000
    },
    MovementDefinition{
        new RandomMovement(),
        new RandomMovement(),
        30'000
    },
    MovementDefinition{
        new VerticalMovement(2500),
        new VerticalMovement(2500, true),
        20'000
    },
    MovementDefinition{
        new StopMovement(4000),
        new StopMovement(4000),
        2'000
    },
    MovementDefinition{
        new RandomMovement(),
        new RandomMovement(),
        45'000
    },
    MovementDefinition{
        new SidewaysMovement(2000),
        new VerticalMovement(2000),
        10'000
    },
    MovementDefinition{
        new VerticalMovement(3000),
        new SidewaysMovement(3000),
        10'000
    },
    MovementDefinition{
        new CircularMovement(3000),
        new CircularMovement(3000, true),
        10'000
    },
    MovementDefinition{
        new SidewaysMovement(1500),
        new SidewaysMovement(1500, true),
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
        5'000
    },
    MovementDefinition{
        new RandomMovement(),
        new RandomMovement(),
        10'000
    }
};

void Animation::init(int leftLR, int leftUD, int rightLR, int rightUD, int startStopButtonPin) {
  enabled = leftLR >= 0 && leftUD >= 0 && rightLR >= 0 && rightUD >= 0;
  if (!enabled) {
    Serial.println("No feelers configured");
    return;
  }
  Serial.println("Feelers configured");
  if (startStopButtonPin >= 0) {
    pinMode(startStopButtonPin, INPUT_PULLUP);
    startStopButton = new Bounce(startStopButtonPin, 10);
  } else {
    animating = true;
  }
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
  if (startStopButton != nullptr && (startStopButton->update() == 1 && startStopButton->fallingEdge())) {
    // The button was pushed, toggle the feeler animatronics off or on
    animating = !animating;
    if (!animating) {
      Serial.println("Feeler button pressed, stopping feelers");
      // Move the feelers towards neutral then stop
      apply(movements[0]);
    } else {
      Serial.println("Feeler button pressed, starting feelers");
      last_movement_change = 0;
    }
  }

  uint32_t now = millis();
  if ((now - last_update) < UPDATE_MS) {
    return;
  }
  last_update = now;
  if (animating && now - last_movement_change > movements[movementIndex].duration) {
    // It's time to switch to the next movement animation
    movementIndex = (movementIndex + 1) % movements.size();
    MovementDefinition md = movements[movementIndex];
    apply(md);
  }
  leftFeeler->update();
  rightFeeler->update();
}

void Animation::apply(const MovementDefinition &md) {
  leftFeeler->setMovement(md.left);
  rightFeeler->setMovement(md.right);
  last_movement_change = millis();
}
