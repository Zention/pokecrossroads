#ifndef GUARD_RANDOMIZER_H
#define GUARD_RANDOMIZER_H

#include "config/randomizer.h"

// Species validity — shared bitmask used by all sub-randomizers
bool8 IsValidSpecies(u16 species);
void  BuildValidSpeciesMask(void);

// Base evolution check — used by starter randomizer pool mode
bool8 IsBaseEvolution(u16 species);
void  BuildBaseEvolutionMask(void);

// Config lifecycle
void InitRandomizerConfig(void);    // call from NewGameInitData
void LoadRandomizerConfig(void);    // call from CB2_ContinueSavedGame
void RestoreRandomizerConfig(void);

// Seed generation (RTC + RNG mix)
u32  RandomizerGenerateSeed(void);

#endif