#ifndef GUARD_RANDOMIZER_ENCOUNTERS_H
#define GUARD_RANDOMIZER_ENCOUNTERS_H

#include "config/randomizer.h"

#define RAND_ENC_LAND_SLOTS     12
#define RAND_ENC_WATER_SLOTS     5
#define RAND_ENC_ROCK_SLOTS      5
#define RAND_ENC_FISHING_SLOTS  10
#define RAND_ENC_HIDDEN_SLOTS    5

// Forward declare the enum so we don't pull in wild_encounter.h here.
// Callers that need the full WildPokemonArea definition include wild_encounter.h directly.
enum WildPokemonArea;

void  RandomizeEncountersForCurrentMap(void);
u16   GetRandomizedEncounterSpecies(enum WildPokemonArea area, u8 slotIndex);
bool8 EncounterRandomizerActive(void);

#endif