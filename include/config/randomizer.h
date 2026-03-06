#ifndef GUARD_CONFIG_RANDOMIZER_H
#define GUARD_CONFIG_RANDOMIZER_H

// Pool modes - what portion of Pokemon can be starters

#define STARTER_POOL_REAL_STARTERS       0  // Only official starter Pokemon 
#define STARTER_POOL_EXTENDED_STARTERS   1  // Real starters + Eevee + Pikachu
#define STARTER_POOL_BASE_EVOLUTIONS     2  // Only Pokemon that have no pre-evolution
#define STARTER_POOL_ALL_POKEMON         3  // Any Pokemon


// Type filter modes
#define STARTER_TYPE_ANY                0xFF  // No type restriction for this slot

// Max starter slots
#define NUM_STARTER_SLOTS               3

// Menu option IDs for the randomizer screen
#define RAND_OPT_POOL_MODE      0
#define RAND_OPT_TYPE_SLOT_1    1
#define RAND_OPT_TYPE_SLOT_2    2
#define RAND_OPT_TYPE_SLOT_3    3
#define RAND_OPT_COUNT          4

struct StarterRandomizerConfig {
	bool8 randomizerEnabled; 		    
    u8 poolMode;                        // STARTER_POOL_* constant
    u8 typeFilter[NUM_STARTER_SLOTS];   // TYPE_* constant or STARTER_TYPE_ANY per slot
    bool8 includeLegendaries;
};

extern struct StarterRandomizerConfig gStarterRandomizerConfig;

#endif