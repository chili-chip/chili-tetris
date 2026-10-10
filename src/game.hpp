#pragma once

#include "32blit.hpp"

// Entry points called by the 32blit runtime.
void init();
void update(uint32_t time);
void render(uint32_t time);
