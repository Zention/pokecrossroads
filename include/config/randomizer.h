#ifndef GUARD_CONFIG_RANDOMIZER_H
#define GUARD_CONFIG_RANDOMIZER_H

// -----------------------------------------------------------------------
// Starter randomizer
// -----------------------------------------------------------------------

#define STARTER_POOL_REAL_STARTERS      0   // Only official starter Pokemon
#define STARTER_POOL_EXTENDED_STARTERS  1   // Real starters + Eevee + Pikachu
#define STARTER_POOL_BASE_EVOLUTIONS    2   // Only Pokemon that have no pre-evolution
#define STARTER_POOL_ALL_POKEMON        3   // Any Pokemon
#define STARTER_POOL_EXTENDED_ORIGINAL  4   // All gen 1-9 starters + hand-picked additions

#define STARTER_TYPE_ANY    0xFF            // No type restriction for this slot
#define NUM_STARTER_SLOTS   3

struct StarterRandomizerConfig {
    bool8 randomizerEnabled;
    u8    poolMode;                         // STARTER_POOL_* constant
    u8    typeFilter[NUM_STARTER_SLOTS];    // TYPE_* constant or STARTER_TYPE_ANY per slot
    bool8 includeLegendaries;
    u32   seed;
};

// -----------------------------------------------------------------------
// Encounter randomizer
// -----------------------------------------------------------------------

struct EncounterRandomizerConfig {
    bool8 randomizerEnabled;
    u32   seed;
};

// -----------------------------------------------------------------------
// Umbrella — the only randomizer field needed in SaveBlock2
// Adding a new sub-randomizer: add its config struct above, add a field
// here, add seed roll in InitRandomizerConfig, restore in LoadRandomizerConfig.
// -----------------------------------------------------------------------

struct RandomizerConfig {
    struct StarterRandomizerConfig  starterConfig;
    struct EncounterRandomizerConfig encounterConfig;
    // struct ItemRandomizerConfig    itemConfig;
    // struct TrainerRandomizerConfig trainerConfig;
};

extern struct RandomizerConfig gRandomizerConfig;

#endif // GUARD_CONFIG_RANDOMIZER_H