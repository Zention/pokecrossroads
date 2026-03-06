#ifndef GUARD_RANDOMIZER_STARTERS_H
#define GUARD_RANDOMIZER_STARTERS_H

#include "config/randomizer.h"

// Declaration only — defined once in randomizer_starters.c
extern struct StarterRandomizerConfig gStarterRandomizerConfig;

bool8 RandomizeStarters(u16 outStarters[NUM_STARTER_SLOTS]);
bool8 RandomizeStartersFromSeed(u32 seed, u16 outStarters[NUM_STARTER_SLOTS]);
void WarmRandomizerCache(void);

#endif