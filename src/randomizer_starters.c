#include "global.h"
#include "pokemon.h"
#include "random.h"
#include "constants/species.h"
#include "config/randomizer.h"
#include "randomizer_starters.h"

static const u16 sRealStarterSpecies[] = {
    SPECIES_BULBASAUR,
    SPECIES_CHARMANDER,
    SPECIES_SQUIRTLE,
    SPECIES_CHIKORITA,
    SPECIES_CYNDAQUIL,
    SPECIES_TOTODILE,
    SPECIES_TREECKO,
    SPECIES_TORCHIC,
    SPECIES_MUDKIP,
    SPECIES_TURTWIG,
    SPECIES_CHIMCHAR,
    SPECIES_PIPLUP,
    SPECIES_SNIVY,
    SPECIES_TEPIG,
    SPECIES_OSHAWOTT,
    SPECIES_CHESPIN,
    SPECIES_FENNEKIN,
    SPECIES_FROAKIE,
    SPECIES_ROWLET,
    SPECIES_LITTEN,
    SPECIES_POPPLIO,
    SPECIES_GROOKEY,
    SPECIES_SCORBUNNY,
    SPECIES_SOBBLE,
    SPECIES_SPRIGATITO,
    SPECIES_FUECOCO,
    SPECIES_QUAXLY
};

static const u16 sExtendedStarterSpecies[] = {
    SPECIES_BULBASAUR,
    SPECIES_CHARMANDER,
    SPECIES_SQUIRTLE,
    SPECIES_CHIKORITA,
    SPECIES_CYNDAQUIL,
    SPECIES_TOTODILE,
    SPECIES_TREECKO,
    SPECIES_TORCHIC,
    SPECIES_MUDKIP,
    SPECIES_TURTWIG,
    SPECIES_CHIMCHAR,
    SPECIES_PIPLUP,
    SPECIES_SNIVY,
    SPECIES_TEPIG,
    SPECIES_OSHAWOTT,
    SPECIES_CHESPIN,
    SPECIES_FENNEKIN,
    SPECIES_FROAKIE,
    SPECIES_ROWLET,
    SPECIES_LITTEN,
    SPECIES_POPPLIO,
    SPECIES_GROOKEY,
    SPECIES_SCORBUNNY,
    SPECIES_SOBBLE,
    SPECIES_SPRIGATITO,
    SPECIES_FUECOCO,
    SPECIES_QUAXLY,
    SPECIES_EEVEE,
    SPECIES_PIKACHU
};

#define NUM_REAL_STARTERS ARRAY_COUNT(sRealStarterSpecies)
#define NUM_EXTENDED_STARTERS ARRAY_COUNT(sExtendedStarterSpecies)

struct StarterRandomizerConfig gStarterRandomizerConfig;

// -------------------------------------------------------
// Bitmask helpers
// -------------------------------------------------------

#define SPECIES_MASK_SIZE ((NUM_SPECIES / 8) + 1)

// Marks species that are base evolutions (no pre-evolution exists).
// Built once by scanning GetSpeciesEvolutions — O(n) build, O(1) lookup.
// Avoids calling GetSpeciesPreEvolution which is itself O(n) per call.
static EWRAM_DATA u8 sBaseEvolutionMask[SPECIES_MASK_SIZE];
static EWRAM_DATA bool8 sBaseEvolutionMaskBuilt;

// Marks species that are the canonical (first) entry for their natDexNum.
// This filters out all form variants: regional forms, Mega evolutions,
// Shadow forms, cosmetic forms (Mothim Sandy/Trash, Scatterbug patterns),
// etc. — regardless of which flags they do or don't have set.
// Built once O(n^2), then O(1) per lookup. Only runs once per power-on.
static EWRAM_DATA u8 sValidSpeciesMask[SPECIES_MASK_SIZE];
static EWRAM_DATA bool8 sValidSpeciesMaskBuilt;

static void BuildBaseEvolutionMask(void)
{
    u16 i;
    u8 j;
    const struct Evolution *evos;
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

static void BuildValidSpeciesMask(void)
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

static bool8 IsBaseEvolution(u16 species)
{
    if (species >= NUM_SPECIES)
        return FALSE;
    return (sBaseEvolutionMask[species / 8] >> (species % 8)) & 1;
}

static bool8 IsValidStarterSpecies(u16 species)
{
    if (species >= NUM_SPECIES)
        return FALSE;
    return (sValidSpeciesMask[species / 8] >> (species % 8)) & 1;
}

// -------------------------------------------------------
// Type / pool helpers
// -------------------------------------------------------

static bool8 SpeciesMatchesType(u16 species, u8 typeFilter)
{
    if (typeFilter == STARTER_TYPE_ANY)
        return TRUE;
    return (gSpeciesInfo[species].types[0] == typeFilter
         || gSpeciesInfo[species].types[1] == typeFilter);
}

static bool8 IsRealStarter(u16 species)
{
    u8 i;
    for (i = 0; i < NUM_REAL_STARTERS; i++)
    {
        if (sRealStarterSpecies[i] == species)
            return TRUE;
    }
    return FALSE;
}

static bool8 IsExtendedStarter(u16 species)
{
    u8 i;
    for (i = 0; i < NUM_EXTENDED_STARTERS; i++)
    {
        if (sExtendedStarterSpecies[i] == species)
            return TRUE;
    }
    return FALSE;
}

// -------------------------------------------------------
// Candidate pool builder
// -------------------------------------------------------

static u16 BuildCandidatePool(u8 typeFilter, u16 *outPool, u16 maxSize)
{
    u16 count;
    u16 species;

    count = 0;
    for (species = 1; species < NUM_SPECIES && count < maxSize; species++)
    {
        // Skip invalid/placeholder species
        if (gSpeciesInfo[species].baseHP == 0)
            continue;

        // Skip form variants — only the canonical base form for each
        // national dex number is allowed. This filters Shadow Lugia,
        // regional forms, Mega evolutions, cosmetic variants, etc.
        if (!IsValidStarterSpecies(species))
            continue;

        // Skip legendaries if the config excludes them
        if (!gStarterRandomizerConfig.includeLegendaries)
        {
            if (gSpeciesInfo[species].isRestrictedLegendary
                || gSpeciesInfo[species].isSubLegendary
                || gSpeciesInfo[species].isMythical
                || gSpeciesInfo[species].isUltraBeast
                || gSpeciesInfo[species].isParadox)
                continue;
        }

        // Pool mode filter
        switch (gStarterRandomizerConfig.poolMode)
        {
        case STARTER_POOL_REAL_STARTERS:
            if (!IsRealStarter(species))
                continue;
            break;
        case STARTER_POOL_BASE_EVOLUTIONS:
            if (!IsBaseEvolution(species))
                continue;
            break;
        case STARTER_POOL_EXTENDED_STARTERS:
            if (!IsExtendedStarter(species))
                continue;
            break;
        case STARTER_POOL_ALL_POKEMON:
        default:
            break;
        }

        // Type filter
        if (!SpeciesMatchesType(species, typeFilter))
            continue;

        outPool[count++] = species;
    }
    return count;
}

// -------------------------------------------------------
// Type overlap check
// -------------------------------------------------------

static bool8 SharesTypeWithChosen(u16 species, u16 *chosenSoFar, u8 numChosen)
{
    u8 i;
    u8 newType1;
    u8 newType2;
    u8 chosenType1;
    u8 chosenType2;

    newType1 = gSpeciesInfo[species].types[0];
    newType2 = gSpeciesInfo[species].types[1];

    for (i = 0; i < numChosen; i++)
    {
        chosenType1 = gSpeciesInfo[chosenSoFar[i]].types[0];
        chosenType2 = gSpeciesInfo[chosenSoFar[i]].types[1];

        if (newType1 == chosenType1) return TRUE;
        if (newType1 == chosenType2) return TRUE;
        if (newType2 != newType1)
        {
            if (newType2 == chosenType1) return TRUE;
            if (newType2 == chosenType2) return TRUE;
        }
    }
    return FALSE;
}

// -------------------------------------------------------
// Core randomizer
// -------------------------------------------------------

bool8 RandomizeStarters(u16 outStarters[NUM_STARTER_SLOTS])
{
    static EWRAM_DATA u16 sCandidatePool[NUM_SPECIES];
    u16 poolSize;
    u16 i;
    u16 j;
    u16 temp;
    u16 pick;
    u8 slot;
    u8 typeFilter;
    u8 prevSlot;
    bool8 found;
    bool8 dupSpecies;

    for (slot = 0; slot < NUM_STARTER_SLOTS; slot++)
    {
        typeFilter = gStarterRandomizerConfig.typeFilter[slot];
        found = FALSE;

        poolSize = BuildCandidatePool(typeFilter, sCandidatePool, NUM_SPECIES);

        if (poolSize == 0)
            return FALSE;

        // Fisher-Yates shuffle
        for (i = poolSize - 1; i > 0; i--)
        {
            j = Random() % (i + 1);
            temp = sCandidatePool[i];
            sCandidatePool[i] = sCandidatePool[j];
            sCandidatePool[j] = temp;
        }

        // Walk shuffled pool, skip duplicates and type overlaps
        for (i = 0; i < poolSize; i++)
        {
            pick = sCandidatePool[i];

            dupSpecies = FALSE;
            for (prevSlot = 0; prevSlot < slot; prevSlot++)
            {
                if (outStarters[prevSlot] == pick)
                {
                    dupSpecies = TRUE;
                    break;
                }
            }
            if (dupSpecies)
                continue;

            if (typeFilter == STARTER_TYPE_ANY)
            {
                if (SharesTypeWithChosen(pick, outStarters, slot))
                    continue;
            }

            found = TRUE;
            break;
        }

        if (!found)
            return FALSE;

        outStarters[slot] = pick;
    }

    return TRUE;
}

bool8 RandomizeStartersFromSeed(u32 seed, u16 outStarters[NUM_STARTER_SLOTS])
{
    rng_value_t savedRngState;
    bool8 result;

    savedRngState = gRngValue;
    SeedRng(seed);
    result = RandomizeStarters(outStarters);
    gRngValue = savedRngState;

    return result;
}

// -------------------------------------------------------
// Cache warmer — call once early (e.g. InitRandomizerConfig)
// to build both bitmasks before they are needed in gameplay.
// -------------------------------------------------------
void WarmRandomizerCache(void)
{
    BuildBaseEvolutionMask();
    BuildValidSpeciesMask();
}