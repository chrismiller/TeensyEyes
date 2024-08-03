#pragma once

#include "Feeler.h"
#include "Movement.h"

class Animation {
  private:
  Feeler* leftFeeler{};
  Feeler* rightFeeler{};
  uint32_t last_update{};
  uint32_t last_movement_change{};
  size_t movementIndex{};
  bool enabled{};
  static constexpr uint16_t UPDATE_MS = 20;

  public:
  void init(int leftLR, int leftUD, int rightLR, int rightUD);

  void update();
};
