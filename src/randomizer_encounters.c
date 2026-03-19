#include "global.h"
#include "randomizer_encounters.h"
#include "wild_encounter.h"
#include "random.h"
#include "overworld.h"
#include "config/randomizer.h"
#include "randomizer.h"

// -----------------------------------------------------------------------
// RAM-only cache — one map at a time, never saved
// -----------------------------------------------------------------------

#define OFFSET_LAND     0
#define OFFSET_WATER    (OFFSET_LAND    + RAND_ENC_LAND_SLOTS)
#define OFFSET_ROCK     (OFFSET_WATER   + RAND_ENC_WATER_SLOTS)
#define OFFSET_FISHING  (OFFSET_ROCK    + RAND_ENC_ROCK_SLOTS)
#define OFFSET_HIDDEN   (OFFSET_FISHING + RAND_ENC_FISHING_SLOTS)
#define RAND_ENC_TOTAL  (OFFSET_HIDDEN  + RAND_ENC_HIDDEN_SLOTS)

struct EncounterCache
{
    u8    mapGroup;
    u8    mapNum;
    u16   species[RAND_ENC_TOTAL];
    bool8 valid;
};

EWRAM_DATA static struct EncounterCache sEncounterCache = { 0 };

// -----------------------------------------------------------------------
// Per-map seed — unique per map, reproducible across visits
// -----------------------------------------------------------------------

static u32 DeriveMapSeed(u8 mapGroup, u8 mapNum)
{
    u32 seed = gRandomizerConfig.encounterConfig.seed;
    // Multiply by primes so (group=1,num=2) != (group=2,num=1)
    seed ^= (u32)mapGroup * 0x00FFF1u;
    seed ^= (u32)mapNum * 0x100003u;
    // xorshift32 to thoroughly mix
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

// -----------------------------------------------------------------------
// Species picker — xorshift32 local state, never touches gRngValue
// -----------------------------------------------------------------------

static u16 PickRandomSpecies(u32* seed)
{
    u16 species;
    u16 attempts = 0;

    do {
        *seed ^= *seed << 13;
        *seed ^= *seed >> 17;
        *seed ^= *seed << 5;
        species = 1 + (u16)(*seed % (NUM_SPECIES - 1));
        attempts++;
        if (attempts > 500)
            return SPECIES_RATTATA;
    } while (!IsValidSpecies(species));

    return species;
}

// -----------------------------------------------------------------------
// Table builder
// -----------------------------------------------------------------------

static void FillSlots(u32* rng, u16* dest, u8 count,
    const struct WildPokemonInfo* info)
{
    u8 i;
    if (info == NULL)
    {
        for (i = 0; i < count; i++)
            dest[i] = SPECIES_NONE;
        return;
    }
    for (i = 0; i < count; i++)
        dest[i] = PickRandomSpecies(rng);
}

static void BuildCacheForMap(u8 mapGroup, u8 mapNum)
{
    u16 headerId = HEADER_NONE;
    u32 rng = DeriveMapSeed(mapGroup, mapNum);
    enum TimeOfDay tod;
    u16 i;

    // Find header
    for (i = 0; ; i++)
    {
        if (gWildMonHeaders[i].mapGroup == MAP_GROUP(MAP_UNDEFINED))
            break;
        if (gWildMonHeaders[i].mapGroup == mapGroup
            && gWildMonHeaders[i].mapNum == mapNum)
        {
            headerId = i;
            break;
        }
    }

    sEncounterCache.mapGroup = mapGroup;
    sEncounterCache.mapNum = mapNum;
    sEncounterCache.valid = FALSE;

    if (headerId == HEADER_NONE)
    {
        // No encounters on this map — fill all with SPECIES_NONE
        memset(sEncounterCache.species, 0, sizeof(sEncounterCache.species));
        sEncounterCache.valid = TRUE;
        return;
    }

    // Fill each area in order — order must never change or seeds shift
    tod = GetTimeOfDayForEncounters(headerId, WILD_AREA_LAND);
    FillSlots(&rng, &sEncounterCache.species[OFFSET_LAND],
        RAND_ENC_LAND_SLOTS,
        gWildMonHeaders[headerId].encounterTypes[tod].landMonsInfo);

    tod = GetTimeOfDayForEncounters(headerId, WILD_AREA_WATER);
    FillSlots(&rng, &sEncounterCache.species[OFFSET_WATER],
        RAND_ENC_WATER_SLOTS,
        gWildMonHeaders[headerId].encounterTypes[tod].waterMonsInfo);

    tod = GetTimeOfDayForEncounters(headerId, WILD_AREA_ROCKS);
    FillSlots(&rng, &sEncounterCache.species[OFFSET_ROCK],
        RAND_ENC_ROCK_SLOTS,
        gWildMonHeaders[headerId].encounterTypes[tod].rockSmashMonsInfo);

    tod = GetTimeOfDayForEncounters(headerId, WILD_AREA_FISHING);
    FillSlots(&rng, &sEncounterCache.species[OFFSET_FISHING],
        RAND_ENC_FISHING_SLOTS,
        gWildMonHeaders[headerId].encounterTypes[tod].fishingMonsInfo);

    tod = GetTimeOfDayForEncounters(headerId, WILD_AREA_HIDDEN);
    FillSlots(&rng, &sEncounterCache.species[OFFSET_HIDDEN],
        RAND_ENC_HIDDEN_SLOTS,
        gWildMonHeaders[headerId].encounterTypes[tod].hiddenMonsInfo);

    sEncounterCache.valid = TRUE;
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

bool8 EncounterRandomizerActive(void)
{
    return gRandomizerConfig.encounterConfig.randomizerEnabled;
}

void RandomizeEncountersForCurrentMap(void)
{
    u8 mapGroup = gSaveBlock1Ptr->location.mapGroup;
    u8 mapNum = gSaveBlock1Ptr->location.mapNum;

    if (!EncounterRandomizerActive())
        return;

    // Cache hit — same map, nothing to do
    if (sEncounterCache.valid
        && sEncounterCache.mapGroup == mapGroup
        && sEncounterCache.mapNum == mapNum)
        return;

    BuildCacheForMap(mapGroup, mapNum);
}

u16 GetRandomizedEncounterSpecies(enum WildPokemonArea area, u8 slotIndex)
{
    u8 offset;

    if (!sEncounterCache.valid)
        return SPECIES_NONE;

    switch (area)
    {
    case WILD_AREA_LAND:    offset = OFFSET_LAND + slotIndex; break;
    case WILD_AREA_WATER:   offset = OFFSET_WATER + slotIndex; break;
    case WILD_AREA_ROCKS:   offset = OFFSET_ROCK + slotIndex; break;
    case WILD_AREA_FISHING: offset = OFFSET_FISHING + slotIndex; break;
    case WILD_AREA_HIDDEN:  offset = OFFSET_HIDDEN + slotIndex; break;
    default:                return SPECIES_NONE;
    }

    return sEncounterCache.species[offset];
}