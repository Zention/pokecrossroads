#include "global.h"
#include "randomizer.h"
#include "random.h"
#include "rtc.h"
#include "randomizer_starters.h"
#include "randomizer_encounters.h"
#include "starter_choose.h"
#include "load_save.h"

// Single global — mirrors gSaveBlock2Ptr->randomizerConfig
struct RandomizerConfig gRandomizerConfig;

// -----------------------------------------------------------------------
// Valid species bitmask — built once, shared by all sub-randomizers
// -----------------------------------------------------------------------

#define SPECIES_MASK_SIZE  ((NUM_SPECIES + 7) / 8)


// Marks species that are the canonical (first) entry for their natDexNum.
// This filters out all form variants: regional forms, Mega evolutions,
// Shadow forms, cosmetic forms (Mothim Sandy/Trash, Scatterbug patterns),
// etc. — regardless of which flags they do or don't have set.
// Built once O(n^2), then O(1) per lookup. Only runs once per power-on.
static EWRAM_DATA u8  sValidSpeciesMask[SPECIES_MASK_SIZE];
static EWRAM_DATA bool8 sValidSpeciesMaskBuilt = FALSE;

// Marks species that are base evolutions (no pre-evolution exists).
// Built once by scanning GetSpeciesEvolutions — O(n) build, O(1) lookup.
// Avoids calling GetSpeciesPreEvolution which is itself O(n) per call.
static EWRAM_DATA u8 sBaseEvolutionMask[SPECIES_MASK_SIZE];
static EWRAM_DATA bool8 sBaseEvolutionMaskBuilt;

void BuildValidSpeciesMask(void)
{
    u16 species;
    u16 earlier;
    bool8 isDuplicate;
    u16 dexNum;

    if (sValidSpeciesMaskBuilt)
        return;

    for (species = 1; species < NUM_SPECIES; species++)
    {
        if (species == 1435)
            continue;

        if (gSpeciesInfo[species].baseHP == 0)
            continue;

        dexNum = gSpeciesInfo[species].natDexNum;

        // Species with no dex number are non-standard entries — skip them
        if (dexNum == NATIONAL_DEX_NONE)
            continue;

        // If an earlier valid species already has this dex number,
        // this species is a form variant — skip it.
        // Base forms always have lower species IDs than their variants.
        isDuplicate = FALSE;
        for (earlier = 1; earlier < species; earlier++)
        {
            if (gSpeciesInfo[earlier].baseHP > 0
                && gSpeciesInfo[earlier].natDexNum == dexNum)
            {
                isDuplicate = TRUE;
                break;
            }
        }

        if (!isDuplicate)
            sValidSpeciesMask[species / 8] |= (1 << (species % 8));
    }

    sValidSpeciesMaskBuilt = TRUE;
}

bool8 IsValidSpecies(u16 species)
{
    if (species >= NUM_SPECIES)
        return FALSE;
    return (sValidSpeciesMask[species / 8] >> (species % 8)) & 1;
}

void BuildBaseEvolutionMask(void)
{
    u16 i;
    u8 j;
    const struct Evolution* evos;
    u16 target;

    if (sBaseEvolutionMaskBuilt)
        return;

    // Start by marking every species as a base evolution
    for (i = 0; i < SPECIES_MASK_SIZE; i++)
        sBaseEvolutionMask[i] = 0xFF;

    // Clear the bit for any species that is an evolution target
    for (i = SPECIES_BULBASAUR; i < NUM_SPECIES; i++)
    {
        if (i == 1435)
            continue;

        evos = GetSpeciesEvolutions(i);
        if (evos == NULL)
            continue;
        for (j = 0; evos[j].method != EVOLUTIONS_END; j++)
        {
            target = SanitizeSpeciesId(evos[j].targetSpecies);
            if (target < NUM_SPECIES)
                sBaseEvolutionMask[target / 8] &= ~(1 << (target % 8));
        }
    }

    sBaseEvolutionMaskBuilt = TRUE;
}

bool8 IsBaseEvolution(u16 species)
{
    if (species >= NUM_SPECIES)
        return FALSE;
    return (sBaseEvolutionMask[species / 8] >> (species % 8)) & 1;
}

// -----------------------------------------------------------------------
// Seed generation
// -----------------------------------------------------------------------

u32 RandomizerGenerateSeed(void)
{
    struct SiiRtcInfo rtc;
    u32 seed;

    RtcGetInfo(&rtc);
    seed = Random32();
    seed ^= (u32)rtc.second;
    seed ^= (u32)rtc.minute << 8;
    seed ^= (u32)rtc.hour << 14;
    seed ^= (u32)rtc.day << 20;
    seed ^= (u32)rtc.month << 26;
    return seed;
}

// -----------------------------------------------------------------------
// Config lifecycle
// -----------------------------------------------------------------------

void InitRandomizerConfig(void)
{

    gRandomizerConfig = gSaveBlock2Ptr->randomizerConfig;

    // Always re-roll seeds on new game
    gRandomizerConfig.starterConfig.seed = RandomizerGenerateSeed();
    gRandomizerConfig.encounterConfig.seed = RandomizerGenerateSeed();

    // Persist to SaveBlock
    gSaveBlock2Ptr->randomizerConfig = gRandomizerConfig;

    // Build shared bitmask then init sub-systems
    BuildValidSpeciesMask();
    BuildBaseEvolutionMask();
    InitStarterMons();
}

void LoadRandomizerConfig(void)
{
    // Restore RAM global from save on continue
    RestoreRandomizerConfig();

    // Rebuild bitmask (RAM is wiped on boot)
    BuildValidSpeciesMask();
    BuildBaseEvolutionMask();

    // Recompute starter slots into sStarterMon[]
    InitStarterMons();

    // Prime encounter cache for the current map
    RandomizeEncountersForCurrentMap();
}

void RestoreRandomizerConfig(void)
{
    if (gSaveFileStatus == SAVE_STATUS_OK || gSaveFileStatus == SAVE_STATUS_ERROR)
        gRandomizerConfig = gSaveBlock2Ptr->randomizerConfig;
    // If no save exists, gRandomizerConfig stays zeroed — that's correct defaults
}