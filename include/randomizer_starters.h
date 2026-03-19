#ifndef GUARD_RANDOMIZER_STARTERS_H
#define GUARD_RANDOMIZER_STARTERS_H

#include "config/randomizer.h"

// Menu option IDs for the randomizer screen
#define RAND_OPT_POOL_MODE      0
#define RAND_OPT_TYPE_SLOT_1    1
#define RAND_OPT_TYPE_SLOT_2    2
#define RAND_OPT_TYPE_SLOT_3    3
#define RAND_OPT_COUNT          4

bool8 RandomizeStarters(u16 outStarters[NUM_STARTER_SLOTS]);
bool8 RandomizeStartersFromSeed(u32 seed, u16 outStarters[NUM_STARTER_SLOTS]);
void WarmRandomizerCache(void);

#endif