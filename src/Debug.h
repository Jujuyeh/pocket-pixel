#pragma once

#include <Arduboy2.h>
#include <Tinyfont.h>

#ifndef POCKET_PIXEL_DEBUG
#define POCKET_PIXEL_DEBUG 0
#endif

bool debugHandleFrame(Arduboy2 &arduboy, Tinyfont &tinyfont);
