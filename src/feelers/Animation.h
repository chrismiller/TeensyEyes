#pragma once

#include <Bounce.h>
#include "Feeler.h"
#include "Movement.h"

struct MovementDefinition {
  Movement *left;
  Movement *right;
  uint32_t duration;
};

class Animation {
  private:
  static constexpr uint16_t UPDATE_MS = 20;
  Feeler* leftFeeler{};
  Feeler* rightFeeler{};
  uint32_t last_update{};
  uint32_t last_movement_change{};
  size_t movementIndex{};
  Bounce *startStopButton{};
  bool enabled{};
  bool animating{};
  void apply(const MovementDefinition &md);

  public:
  void init(int leftLR, int leftUD, int rightLR, int rightUD, int startStopButtonPin);

  void update();
};
