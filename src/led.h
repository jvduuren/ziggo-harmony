#pragma once

#include <Arduino.h>

namespace led {

enum class Mode { Off, Advertising, Connected, Learning };

void begin();
void selfTest();  // red, green, blue: shows whether LED_SWAP_RG is right for the board
void setMode(Mode mode);
void flash(uint8_t r, uint8_t g, uint8_t b, uint16_t ms);
void findMe(uint32_t ms);  // "find my remote" request from the box
void update();

}  // namespace led
