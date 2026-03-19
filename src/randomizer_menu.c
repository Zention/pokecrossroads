#include "global.h"
#include "randomizer_menu.h"
#include "randomizer_starters.h"
#include "config/randomizer.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "save.h"
#include "constants/pokemon.h"
#include "constants/rgb.h"
#include "starter_choose.h"
#include "random.h"
#include "rtc.h"

// -------------------------------------------------------
// Task data aliases
// -------------------------------------------------------

// STARTER RANDOMIZER CONFIG
#define tMenuSelection                          data[0]
#define tStarterRandPoolMode                    data[1]
#define tStarterRandTypeSlot1                   data[2]
#define tStarterRandTypeSlot2                   data[3]
#define tStarterRandTypeSlot3                   data[4]
#define tStarterRandEnabled                     data[5]
#define tStarterRandIncludeLegendaries          data[6]
#define tPage                                   data[7]

// ENCOUNTER RANDOMIZER CONFIG
#define tEncounterRandEnabled                   data[8]
// -------------------------------------------------------
// Menu item indices
// -------------------------------------------------------

// Page 1 — Starter Randomizer
enum
{
    MENUITEM_P1_STARTER_RAND_ENABLED,
    MENUITEM_P1_STARTER_RAND_POOL,
    MENUITEM_P1_STARTER_RAND_TYPE1,
    MENUITEM_P1_STARTER_RAND_TYPE2,
    MENUITEM_P1_STARTER_RAND_TYPE3,
    MENUITEM_P1_STARTER_RAND_INCLUDE_LEGENDARIES,
    MENUITEM_P1_NEXT,
    MENUITEM_P1_COUNT,
};

// Page 2 — Encounter Randomizer
enum
{
    MENUITEM_P2_ENCOUNTER_RAND_ENABLED,
    MENUITEM_P2_PREV,       // back to page 1
    MENUITEM_CANCEL,     // save and exit
    MENUITEM_P2_COUNT,
};

enum
{
    WIN_RAND_HEADER,
    WIN_RAND_OPTIONS
};

#define YPOS_STARTER_RAND_ENABLED            (MENUITEM_P1_STARTER_RAND_ENABLED * 16)
#define YPOS_STARTER_RAND_POOL               (MENUITEM_P1_STARTER_RAND_POOL  * 16)
#define YPOS_STARTER_RAND_TYPE1              (MENUITEM_P1_STARTER_RAND_TYPE1 * 16)
#define YPOS_STARTER_RAND_TYPE2              (MENUITEM_P1_STARTER_RAND_TYPE2 * 16)
#define YPOS_STARTER_RAND_TYPE3              (MENUITEM_P1_STARTER_RAND_TYPE3 * 16)
#define YPOS_STARTER_RAND_LEGENDARIES        (MENUITEM_P1_STARTER_RAND_INCLUDE_LEGENDARIES    * 16)

#define YPOS_ENCOUNTER_RAND_ENABLED          (MENUITEM_P2_ENCOUNTER_RAND_ENABLED * 16)

// -------------------------------------------------------
// Forward declarations
// -------------------------------------------------------

static void Task_RandomizerMenuFadeIn(u8 taskId);
static void Task_RandomizerMenuProcessInput(u8 taskId);
static void Task_RandomizerMenuSave(u8 taskId);
static void Task_RandomizerMenuFadeOut(u8 taskId);
static void HighlightRandomizerMenuItem(u8 selection);
static void DrawRandomizerHeaderText(void);
static void DrawRandomizerMenuTexts(void);
static void DrawRandomizerBgWindowFrames(void);
static void DrawStarterRandEnabledChoices(bool8 enabled);
static void DrawPoolModeChoices(u8 selection, bool8 locked);
static void DrawTypeSlotChoices(u8 slotMenuItem, u8 typeValue, bool8 locked);
static void DrawIncludeLegendariesChoices(bool8 included, bool8 locked);
static void DrawEncounterRandEnabledChoices(bool8 enabled);
static u8 CyclePoolMode(u8 current, s8 dir);
static u8 CycleTypeFilter(u8 current, s8 dir);
static bool8 TypeSlotsAreLocked(u8 taskId);
static bool8 AllOptionsLocked(u8 taskId);
static bool8 LegendariesLocked(u8 taskId);
static void SwitchToPage(u8 taskId, u8 page);
static void HandlePage1Input(u8 taskId);
static void HandlePage2Input(u8 taskId);
static void DrawPageItems(u8 taskId);
static void DrawPageValues(u8 taskId);

// -------------------------------------------------------
// Type name strings + lookup tables
// -------------------------------------------------------

static const u8 sText_TypeAny[] = _("Any");
static const u8 sText_TypeNormal[] = _("Normal");
static const u8 sText_TypeFighting[] = _("Fighting");
static const u8 sText_TypeFlying[] = _("Flying");
static const u8 sText_TypePoison[] = _("Poison");
static const u8 sText_TypeGround[] = _("Ground");
static const u8 sText_TypeRock[] = _("Rock");
static const u8 sText_TypeBug[] = _("Bug");
static const u8 sText_TypeGhost[] = _("Ghost");
static const u8 sText_TypeSteel[] = _("Steel");
static const u8 sText_TypeFire[] = _("Fire");
static const u8 sText_TypeWater[] = _("Water");
static const u8 sText_TypeGrass[] = _("Grass");
static const u8 sText_TypeElectric[] = _("Electric");
static const u8 sText_TypePsychic[] = _("Psychic");
static const u8 sText_TypeIce[] = _("Ice");
static const u8 sText_TypeDragon[] = _("Dragon");
static const u8 sText_TypeDark[] = _("Dark");

static const u8 sTypeValues[] = {
    STARTER_TYPE_ANY,
    TYPE_NORMAL,
    TYPE_FIGHTING,
    TYPE_FLYING,
    TYPE_POISON,
    TYPE_GROUND,
    TYPE_ROCK,
    TYPE_BUG,
    TYPE_GHOST,
    TYPE_STEEL,
    TYPE_FIRE,
    TYPE_WATER,
    TYPE_GRASS,
    TYPE_ELECTRIC,
    TYPE_PSYCHIC,
    TYPE_ICE,
    TYPE_DRAGON,
    TYPE_DARK,
};

static const u8* const sTypeNames[] = {
    sText_TypeAny,
    sText_TypeNormal,
    sText_TypeFighting,
    sText_TypeFlying,
    sText_TypePoison,
    sText_TypeGround,
    sText_TypeRock,
    sText_TypeBug,
    sText_TypeGhost,
    sText_TypeSteel,
    sText_TypeFire,
    sText_TypeWater,
    sText_TypeGrass,
    sText_TypeElectric,
    sText_TypePsychic,
    sText_TypeIce,
    sText_TypeDragon,
    sText_TypeDark,
};

#define NUM_TYPE_OPTIONS ARRAY_COUNT(sTypeValues)

// -------------------------------------------------------
// Pool mode strings
// -------------------------------------------------------

static const u8 sText_Pool_Any[] = _("Any Pokemon");
static const u8 sText_Pool_RealStarters[] = _("Original Starters");
static const u8 sText_Pool_ExtendedStarters[] = _("Extended Originals");
static const u8 sText_Pool_BaseOnly[] = _("Base Evolutions");

static const u8* const sPoolModeNames[] = {
    sText_Pool_RealStarters,
    sText_Pool_ExtendedStarters,
    sText_Pool_BaseOnly,
    sText_Pool_Any,
};

#define NUM_POOL_MODES ARRAY_COUNT(sPoolModeNames)

// -------------------------------------------------------
// Menu item labels
// -------------------------------------------------------

// General page labels and universal options
static const u8 sText_RandHeader_P1[] = _("RANDOMIZER SETTINGS (1/2)");
static const u8 sText_RandHeader_P2[] = _("RANDOMIZER SETTINGS (2/2)");
static const u8 sText_ValOn[] = _("ON");
static const u8 sText_ValOff[] = _("OFF");
static const u8 sText_Locked[] = _("---");
static const u8 sText_OptCancel[] = _("RETURN TO MAIN MENU");
static const u8 sText_P1_Next[] = _("ENCOUNTERS >");
static const u8 sText_P2_Prev[] = _("< STARTERS");

// Starter randomizer labels
static const u8 sText_OptStarterRandEnabled[] = _("STARTER RANDOMIZER");
static const u8 sText_OptStarterRandPool[] = _("STARTER POOL");
static const u8 sText_OptStarterRandTypeSlot1[] = _("TYPE SLOT 1");
static const u8 sText_OptStarterRandTypeSlot2[] = _("TYPE SLOT 2");
static const u8 sText_OptStarterRandTypeSlot3[] = _("TYPE SLOT 3");
static const u8 sText_OptStarterRandIncludeLegendaries[] = _("INCLUDE LEGENDARIES");

// Encounter randomizer labels
static const u8 sText_P2_EncRandEnabled[] = _("ENC. RANDOMIZER");

// Page 1 - Starter Randomizer
static const u8* const sPage1ItemNames[MENUITEM_P1_COUNT] =
{
    [MENUITEM_P1_STARTER_RAND_ENABLED] = sText_OptStarterRandEnabled,
    [MENUITEM_P1_STARTER_RAND_POOL] = sText_OptStarterRandPool,
    [MENUITEM_P1_STARTER_RAND_TYPE1] = sText_OptStarterRandTypeSlot1,
    [MENUITEM_P1_STARTER_RAND_TYPE2] = sText_OptStarterRandTypeSlot2,
    [MENUITEM_P1_STARTER_RAND_TYPE3] = sText_OptStarterRandTypeSlot3,
    [MENUITEM_P1_STARTER_RAND_INCLUDE_LEGENDARIES] = sText_OptStarterRandIncludeLegendaries,
    [MENUITEM_P1_NEXT] = sText_P1_Next,
};

// Page 2 - Encounter Randomizer
static const u8* const sPage2ItemNames[MENUITEM_P2_COUNT] =
{
    [MENUITEM_P2_ENCOUNTER_RAND_ENABLED] = sText_P2_EncRandEnabled,
    [MENUITEM_P2_PREV] = sText_P2_Prev,
    [MENUITEM_CANCEL] = sText_OptCancel,
};
// -------------------------------------------------------
// Window + BG templates  (mirrors option_menu.c exactly)
// -------------------------------------------------------

static const struct WindowTemplate sRandMenuWinTemplates[] =
{
    [WIN_RAND_HEADER] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    [WIN_RAND_OPTIONS] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 26,
        .height = 16,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sRandMenuBgTemplates[] =
{
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 0,
        .charBaseIndex = 1,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    }
};

static const u16 sRandMenuBg_Pal[] = { RGB(17, 18, 31) };

// Reuse option_menu's text palette — keeps visual consistency
static const u16 sRandMenuText_Pal[] = INCBIN_U16("graphics/interface/option_menu_text.gbapal");

// -------------------------------------------------------
// Frame tile constants (same as option_menu.c)
// -------------------------------------------------------

#define TILE_TOP_CORNER_L 0x1A2
#define TILE_TOP_EDGE     0x1A3
#define TILE_TOP_CORNER_R 0x1A4
#define TILE_LEFT_EDGE    0x1A5
#define TILE_RIGHT_EDGE   0x1A7
#define TILE_BOT_CORNER_L 0x1A8
#define TILE_BOT_EDGE     0x1A9
#define TILE_BOT_CORNER_R 0x1AA

// -------------------------------------------------------
// Per-frame callbacks (mirrors option_menu.c MainCB2/VBlankCB)
// -------------------------------------------------------

static void RandMenuMainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void RandMenuVBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// -------------------------------------------------------
// CB2_RandomizerMenu — state machine init (mirrors CB2_InitOptionMenu)
// -------------------------------------------------------

void CB2_RandomizerMenu(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void*)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sRandMenuBgTemplates, ARRAY_COUNT(sRandMenuBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0, BG_COORD_SET);
        InitWindows(sRandMenuWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_CLR);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_DARKEN);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 4);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        gMain.state++;
        break;
    case 3:
        LoadBgTiles(1, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, 0x1A2);
        gMain.state++;
        break;
    case 4:
        LoadPalette(sRandMenuBg_Pal, BG_PLTT_ID(0), sizeof(sRandMenuBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        // Reuse the option menu text palette for visual consistency
        LoadPalette(sRandMenuText_Pal, BG_PLTT_ID(1), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(WIN_RAND_HEADER);
        DrawRandomizerHeaderText();
        gMain.state++;
        break;
    case 7:
        gMain.state++;
        break;
    case 8:
        PutWindowTilemap(WIN_RAND_OPTIONS);
        DrawRandomizerMenuTexts();
        gMain.state++;
        // fallthrough
    case 9:
        DrawRandomizerBgWindowFrames();
        gMain.state++;
        break;
    case 10:
    {
        u8 taskId = CreateTask(Task_RandomizerMenuFadeIn, 0);

        // Load current config into task data as working copies
        gTasks[taskId].tStarterRandEnabled = gRandomizerConfig.starterConfig.randomizerEnabled;
        gTasks[taskId].tMenuSelection = 0;
        gTasks[taskId].tStarterRandPoolMode = gRandomizerConfig.starterConfig.poolMode;
        gTasks[taskId].tStarterRandTypeSlot1 = gRandomizerConfig.starterConfig.typeFilter[0];
        gTasks[taskId].tStarterRandTypeSlot2 = gRandomizerConfig.starterConfig.typeFilter[1];
        gTasks[taskId].tStarterRandTypeSlot3 = gRandomizerConfig.starterConfig.typeFilter[2];
        gTasks[taskId].tStarterRandIncludeLegendaries = gRandomizerConfig.starterConfig.includeLegendaries;
		gTasks[taskId].tEncounterRandEnabled = gRandomizerConfig.encounterConfig.randomizerEnabled;

        // Draw initial values
        DrawStarterRandEnabledChoices(gTasks[taskId].tStarterRandEnabled);
        DrawPoolModeChoices(gTasks[taskId].tStarterRandPoolMode, AllOptionsLocked(taskId));
        DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE1, gTasks[taskId].tStarterRandTypeSlot1, TypeSlotsAreLocked(taskId));
        DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE2, gTasks[taskId].tStarterRandTypeSlot2, TypeSlotsAreLocked(taskId));
        DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE3, gTasks[taskId].tStarterRandTypeSlot3, TypeSlotsAreLocked(taskId));
        DrawIncludeLegendariesChoices(gTasks[taskId].tStarterRandIncludeLegendaries, LegendariesLocked(taskId));
        HighlightRandomizerMenuItem(gTasks[taskId].tMenuSelection);

        CopyWindowToVram(WIN_RAND_OPTIONS, COPYWIN_FULL);
        gMain.state++;
        break;
    }
    case 11:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(RandMenuVBlankCB);
        SetMainCallback2(RandMenuMainCB2);
        return;
    }
}

// -------------------------------------------------------
// Tasks
// -------------------------------------------------------

static void Task_RandomizerMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_RandomizerMenuProcessInput;
}

static void Task_RandomizerMenuProcessInput(u8 taskId)
{
    if (gTasks[taskId].tPage == 0)
        HandlePage1Input(taskId);
    else
        HandlePage2Input(taskId);
}

static void Task_RandomizerMenuSave(u8 taskId)
{
    struct SiiRtcInfo rtc;
    u32 seed;

    WarmRandomizerCache();

    // Build the complete config from task working copies
    gRandomizerConfig.starterConfig.randomizerEnabled = gTasks[taskId].tStarterRandEnabled;
    gRandomizerConfig.starterConfig.poolMode = gTasks[taskId].tStarterRandPoolMode;
    gRandomizerConfig.starterConfig.includeLegendaries = gTasks[taskId].tStarterRandIncludeLegendaries;

    if (gTasks[taskId].tStarterRandPoolMode == STARTER_POOL_REAL_STARTERS)
    {
        gRandomizerConfig.starterConfig.typeFilter[0] = TYPE_GRASS;
        gRandomizerConfig.starterConfig.typeFilter[1] = TYPE_FIRE;
        gRandomizerConfig.starterConfig.typeFilter[2] = TYPE_WATER;
    }
    else if (gTasks[taskId].tStarterRandPoolMode == STARTER_POOL_EXTENDED_STARTERS)
    {
        gRandomizerConfig.starterConfig.typeFilter[0] = STARTER_TYPE_ANY;
        gRandomizerConfig.starterConfig.typeFilter[1] = STARTER_TYPE_ANY;
        gRandomizerConfig.starterConfig.typeFilter[2] = STARTER_TYPE_ANY;
    }
    else
    {
        gRandomizerConfig.starterConfig.typeFilter[0] = gTasks[taskId].tStarterRandTypeSlot1;
        gRandomizerConfig.starterConfig.typeFilter[1] = gTasks[taskId].tStarterRandTypeSlot2;
        gRandomizerConfig.starterConfig.typeFilter[2] = gTasks[taskId].tStarterRandTypeSlot3;
    }

    gRandomizerConfig.encounterConfig.randomizerEnabled = gTasks[taskId].tEncounterRandEnabled;


    // Always re-roll seed and re-cache starters, enabled or not.
    // When disabled, InitStarterMons returns early — no harm done.
    // When enabled, the cache is freshly populated for the next new game.
    RtcGetInfo(&rtc);
    seed = Random32();
    seed ^= (u32)rtc.second;
    seed ^= (u32)rtc.minute << 8;
    seed ^= (u32)rtc.hour << 14;
    seed ^= (u32)rtc.day << 20;
    seed ^= (u32)rtc.month << 26;
    gRandomizerConfig.starterConfig.seed = seed;
    gSaveBlock2Ptr->randomizerConfig = gRandomizerConfig;

    InitStarterMons();

    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_RandomizerMenuFadeOut;
}

static void Task_RandomizerMenuFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}

// -------------------------------------------------------
// Highlight (mirrors HighlightOptionMenuItem exactly)
// -------------------------------------------------------

static void HighlightRandomizerMenuItem(u8 index)
{
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(16, DISPLAY_WIDTH - 16));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(index * 16 + 40, index * 16 + 56));
}

// -------------------------------------------------------
// Draw helpers
// -------------------------------------------------------

static u8 TypeValueToIndex(u8 typeValue)
{
    u8 i;
    for (i = 0; i < NUM_TYPE_OPTIONS; i++)
    {
        if (sTypeValues[i] == typeValue)
            return i;
    }
    return 0;
}

static u8 CyclePoolMode(u8 current, s8 dir)
{
    return (u8)((current + NUM_POOL_MODES + dir) % NUM_POOL_MODES);
}

static u8 CycleTypeFilter(u8 current, s8 dir)
{
    u8 idx = TypeValueToIndex(current);
    idx = (u8)((idx + NUM_TYPE_OPTIONS + dir) % NUM_TYPE_OPTIONS);
    return sTypeValues[idx];
}

static void DrawStarterRandEnabledChoices(bool8 enabled)
{
    FillWindowPixelRect(WIN_RAND_OPTIONS, PIXEL_FILL(1), 140, YPOS_STARTER_RAND_ENABLED, 104, 16);
    AddTextPrinterParameterized(WIN_RAND_OPTIONS, FONT_NORMAL,
        enabled ? sText_ValOn : sText_ValOff,
        140, YPOS_STARTER_RAND_ENABLED + 1, TEXT_SKIP_DRAW, NULL);
}

// Draws the current pool mode value on the right side of its row
static void DrawPoolModeChoices(u8 selection, bool8 locked)
{
    // Clear the value area (right half of the row)
    FillWindowPixelRect(WIN_RAND_OPTIONS, PIXEL_FILL(1), 104, YPOS_STARTER_RAND_POOL, 104, 16);
    // When real starters mode is active, show "---" instead of the type name
    // to make clear these slots are not configurable
    if (locked)
    {
        AddTextPrinterParameterized(WIN_RAND_OPTIONS, FONT_NORMAL,
            sText_Locked,
            104, YPOS_STARTER_RAND_POOL + 1,
            TEXT_SKIP_DRAW, NULL);
    }
    else
    {
        AddTextPrinterParameterized(WIN_RAND_OPTIONS, FONT_NORMAL,
            sPoolModeNames[selection],
            104, YPOS_STARTER_RAND_POOL + 1,
            TEXT_SKIP_DRAW, NULL);
    }
}

// Draws the current type value for the given slot menu item row
static void DrawTypeSlotChoices(u8 slotMenuItem, u8 typeValue, bool8 locked)
{
    u8 ypos;
    u8 idx;
    

    switch (slotMenuItem)
    {
    case MENUITEM_P1_STARTER_RAND_TYPE1:  ypos = YPOS_STARTER_RAND_TYPE1;  break;
    case MENUITEM_P1_STARTER_RAND_TYPE2:  ypos = YPOS_STARTER_RAND_TYPE2;  break;
    case MENUITEM_P1_STARTER_RAND_TYPE3:  ypos = YPOS_STARTER_RAND_TYPE3;  break;
    default:                   return;
    }

    FillWindowPixelRect(WIN_RAND_OPTIONS, PIXEL_FILL(1), 104, ypos, 104, 16);

    // When real starters mode is active, show "---" instead of the type name
    // to make clear these slots are not configurable
    if (locked)
    {
        AddTextPrinterParameterized(WIN_RAND_OPTIONS, FONT_NORMAL,
            sText_Locked, 104, ypos + 1, TEXT_SKIP_DRAW, NULL);
    }
    else
    {
        idx = TypeValueToIndex(typeValue);
        AddTextPrinterParameterized(WIN_RAND_OPTIONS, FONT_NORMAL,
            sTypeNames[idx], 104, ypos + 1, TEXT_SKIP_DRAW, NULL);
    }
}

static void DrawRandomizerHeaderText(void)
{
    FillWindowPixelBuffer(WIN_RAND_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_RAND_HEADER, FONT_NORMAL, sText_RandHeader_P1, 8, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_RAND_HEADER, COPYWIN_FULL);
}

static void DrawRandomizerMenuTexts(void)
{
    u8 i;

    FillWindowPixelBuffer(WIN_RAND_OPTIONS, PIXEL_FILL(1));
    for (i = 0; i < MENUITEM_P1_COUNT; i++)
        AddTextPrinterParameterized(WIN_RAND_OPTIONS, FONT_SMALL,
            sPage1ItemNames[i],
            8, (i * 16) + 1,
            TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_RAND_OPTIONS, COPYWIN_FULL);
}

static void DrawRandomizerBgWindowFrames(void)
{
    // Title window frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L, 1, 0, 1, 1, 7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE, 2, 0, 27, 1, 7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28, 0, 1, 1, 7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE, 1, 1, 1, 2, 7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE, 28, 1, 1, 2, 7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L, 1, 3, 1, 1, 7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE, 2, 3, 27, 1, 7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 3, 1, 1, 7);

    // Options list window frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L, 1, 4, 1, 1, 7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE, 2, 4, 26, 1, 7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28, 4, 1, 1, 7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE, 1, 5, 1, 16, 7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE, 28, 5, 1, 16, 7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L, 1, 21, 1, 1, 7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE, 2, 21, 26, 1, 7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 21, 1, 1, 7);

    CopyBgTilemapBufferToVram(1);
}

static void DrawIncludeLegendariesChoices(bool8 included, bool8 locked)
{
    FillWindowPixelRect(WIN_RAND_OPTIONS, PIXEL_FILL(1), 104, YPOS_STARTER_RAND_LEGENDARIES, 104, 16);
    if (locked)
        AddTextPrinterParameterized(WIN_RAND_OPTIONS, FONT_NORMAL,
            sText_Locked, 104, YPOS_STARTER_RAND_LEGENDARIES + 1, TEXT_SKIP_DRAW, NULL);
    else
        AddTextPrinterParameterized(WIN_RAND_OPTIONS, FONT_NORMAL,
            included ? sText_ValOn : sText_ValOff,
            104, YPOS_STARTER_RAND_LEGENDARIES + 1, TEXT_SKIP_DRAW, NULL);
}

// Encounter Randomization Options
static void DrawEncounterRandEnabledChoices(bool8 enabled)
{
    FillWindowPixelRect(WIN_RAND_OPTIONS, PIXEL_FILL(1), 140, YPOS_ENCOUNTER_RAND_ENABLED, 104, 16);
    AddTextPrinterParameterized(WIN_RAND_OPTIONS, FONT_NORMAL,
        enabled ? sText_ValOn : sText_ValOff,
        140, YPOS_ENCOUNTER_RAND_ENABLED + 1, TEXT_SKIP_DRAW, NULL);
}

/// Option Locking Helper Functions

static bool8 AllOptionsLocked(u8 taskId)
{
    return gTasks[taskId].tStarterRandEnabled == FALSE;
}

static bool8 TypeSlotsAreLocked(u8 taskId)
{
    return AllOptionsLocked(taskId)
        || gTasks[taskId].tStarterRandPoolMode == STARTER_POOL_REAL_STARTERS
        || gTasks[taskId].tStarterRandPoolMode == STARTER_POOL_EXTENDED_STARTERS;
}

static bool8 LegendariesLocked(u8 taskId)
{
    return AllOptionsLocked(taskId)
        || gTasks[taskId].tStarterRandPoolMode == STARTER_POOL_REAL_STARTERS
        || gTasks[taskId].tStarterRandPoolMode == STARTER_POOL_EXTENDED_STARTERS;
}

// Page Switching Helper Functions
static void SwitchToPage(u8 taskId, u8 page)
{
    gTasks[taskId].tPage = page;
    gTasks[taskId].tMenuSelection = 0;

    // Redraw header to show new page number
    FillWindowPixelBuffer(WIN_RAND_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_RAND_HEADER, FONT_NORMAL,
        page == 0 ? sText_RandHeader_P1 : sText_RandHeader_P2,
        8, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_RAND_HEADER, COPYWIN_FULL);

    // Redraw options window for new page
    DrawPageItems(taskId);
    DrawPageValues(taskId);
    HighlightRandomizerMenuItem(0);
    CopyWindowToVram(WIN_RAND_OPTIONS, COPYWIN_FULL);
}

static void HandlePage1Input(u8 taskId)
{
    u8 prev;

    if (JOY_NEW(A_BUTTON))
    {
        if (gTasks[taskId].tMenuSelection == MENUITEM_P1_NEXT)
            SwitchToPage(taskId, 1);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_RandomizerMenuSave;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (gTasks[taskId].tMenuSelection > 0)
        {
            gTasks[taskId].tMenuSelection--;

            // If everything is locked, skip straight to ENABLED row
            if (AllOptionsLocked(taskId)
                && gTasks[taskId].tMenuSelection > MENUITEM_P1_STARTER_RAND_ENABLED
                && gTasks[taskId].tMenuSelection < MENUITEM_P1_NEXT)
            {
                gTasks[taskId].tMenuSelection = MENUITEM_P1_STARTER_RAND_ENABLED;
            }
            // If only type slots are locked, skip over them
            else if (TypeSlotsAreLocked(taskId)
                && gTasks[taskId].tMenuSelection >= MENUITEM_P1_STARTER_RAND_TYPE1
                && gTasks[taskId].tMenuSelection <= MENUITEM_P1_STARTER_RAND_TYPE3)
            {
                gTasks[taskId].tMenuSelection = MENUITEM_P1_STARTER_RAND_POOL;
            }
        }
        else
        {
            gTasks[taskId].tMenuSelection = MENUITEM_P1_NEXT;
        }
        HighlightRandomizerMenuItem(gTasks[taskId].tMenuSelection);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (gTasks[taskId].tMenuSelection < MENUITEM_P1_NEXT)
        {
            gTasks[taskId].tMenuSelection++;

            // If everything is locked, skip from ENABLED straight to CANCEL
            if (AllOptionsLocked(taskId)
                && gTasks[taskId].tMenuSelection > MENUITEM_P1_STARTER_RAND_ENABLED
                && gTasks[taskId].tMenuSelection < MENUITEM_P1_NEXT)
            {
                gTasks[taskId].tMenuSelection = MENUITEM_P1_NEXT;
            }
            // If only type slots are locked, skip over them
            else if (TypeSlotsAreLocked(taskId)
                && gTasks[taskId].tMenuSelection >= MENUITEM_P1_STARTER_RAND_TYPE1
                && gTasks[taskId].tMenuSelection <= MENUITEM_P1_STARTER_RAND_TYPE3)
            {
                gTasks[taskId].tMenuSelection = MENUITEM_P1_NEXT;
            }
        }
        else
        {
            gTasks[taskId].tMenuSelection = 0;
        }
        HighlightRandomizerMenuItem(gTasks[taskId].tMenuSelection);
    }
    else if (JOY_NEW(DPAD_LEFT) || JOY_NEW(DPAD_RIGHT))
    {
        s8 dir = JOY_NEW(DPAD_RIGHT) ? +1 : -1;

        switch (gTasks[taskId].tMenuSelection)
        {
        case MENUITEM_P1_STARTER_RAND_ENABLED:
            // Toggling enabled/disabled is always allowed
            gTasks[taskId].tStarterRandEnabled = !gTasks[taskId].tStarterRandEnabled;
            DrawStarterRandEnabledChoices(gTasks[taskId].tStarterRandEnabled);
            // Redraw all other rows to show locked/unlocked state
            DrawPoolModeChoices(gTasks[taskId].tStarterRandPoolMode, AllOptionsLocked(taskId));
            DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE1, gTasks[taskId].tStarterRandTypeSlot1, TypeSlotsAreLocked(taskId));
            DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE2, gTasks[taskId].tStarterRandTypeSlot2, TypeSlotsAreLocked(taskId));
            DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE3, gTasks[taskId].tStarterRandTypeSlot3, TypeSlotsAreLocked(taskId));
            DrawIncludeLegendariesChoices(gTasks[taskId].tStarterRandIncludeLegendaries, LegendariesLocked(taskId));
            break;
        case MENUITEM_P1_STARTER_RAND_POOL:
            prev = gTasks[taskId].tStarterRandPoolMode;
            gTasks[taskId].tStarterRandPoolMode = CyclePoolMode(gTasks[taskId].tStarterRandPoolMode, dir);
            if (prev != gTasks[taskId].tStarterRandPoolMode)
            {
                DrawPoolModeChoices(gTasks[taskId].tStarterRandPoolMode, AllOptionsLocked(taskId));
                // Redraw all type slots to show locked/unlocked state
                DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE1, gTasks[taskId].tStarterRandTypeSlot1, TypeSlotsAreLocked(taskId));
                DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE2, gTasks[taskId].tStarterRandTypeSlot2, TypeSlotsAreLocked(taskId));
                DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE3, gTasks[taskId].tStarterRandTypeSlot3, TypeSlotsAreLocked(taskId));
            }
            break;
        case MENUITEM_P1_STARTER_RAND_TYPE1:
            // Ignore input on type slots when locked
            if (TypeSlotsAreLocked(taskId))
                return;
            prev = gTasks[taskId].tStarterRandTypeSlot1;
            gTasks[taskId].tStarterRandTypeSlot1 = CycleTypeFilter(gTasks[taskId].tStarterRandTypeSlot1, dir);
            if (prev != gTasks[taskId].tStarterRandTypeSlot1)
                DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE1, gTasks[taskId].tStarterRandTypeSlot1, TypeSlotsAreLocked(taskId));
            break;
        case MENUITEM_P1_STARTER_RAND_TYPE2:
            // Ignore input on type slots when locked
            if (TypeSlotsAreLocked(taskId))
                return;
            prev = gTasks[taskId].tStarterRandTypeSlot2;
            gTasks[taskId].tStarterRandTypeSlot2 = CycleTypeFilter(gTasks[taskId].tStarterRandTypeSlot2, dir);
            if (prev != gTasks[taskId].tStarterRandTypeSlot2)
                DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE2, gTasks[taskId].tStarterRandTypeSlot2, TypeSlotsAreLocked(taskId));
            break;
        case MENUITEM_P1_STARTER_RAND_TYPE3:
            // Ignore input on type slots when locked
            if (TypeSlotsAreLocked(taskId))
                return;
            prev = gTasks[taskId].tStarterRandTypeSlot3;
            gTasks[taskId].tStarterRandTypeSlot3 = CycleTypeFilter(gTasks[taskId].tStarterRandTypeSlot3, dir);
            if (prev != gTasks[taskId].tStarterRandTypeSlot3)
                DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE3, gTasks[taskId].tStarterRandTypeSlot3, TypeSlotsAreLocked(taskId));
            break;
        case MENUITEM_P1_STARTER_RAND_INCLUDE_LEGENDARIES:
            if (LegendariesLocked(taskId))
                return;
            gTasks[taskId].tStarterRandIncludeLegendaries = !gTasks[taskId].tStarterRandIncludeLegendaries;
            DrawIncludeLegendariesChoices(gTasks[taskId].tStarterRandIncludeLegendaries, LegendariesLocked(taskId));
            break;
        default:
            return;
        }
        CopyWindowToVram(WIN_RAND_OPTIONS, COPYWIN_GFX);
    }
}

static void HandlePage2Input(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        if (gTasks[taskId].tMenuSelection == MENUITEM_P2_PREV)
            SwitchToPage(taskId, 0);
        else if (gTasks[taskId].tMenuSelection == MENUITEM_CANCEL)
            gTasks[taskId].func = Task_RandomizerMenuSave;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_RandomizerMenuSave;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (gTasks[taskId].tMenuSelection > 0)
            gTasks[taskId].tMenuSelection--;
        else
            gTasks[taskId].tMenuSelection = MENUITEM_CANCEL;
        HighlightRandomizerMenuItem(gTasks[taskId].tMenuSelection);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (gTasks[taskId].tMenuSelection < MENUITEM_CANCEL)
            gTasks[taskId].tMenuSelection++;
        else
            gTasks[taskId].tMenuSelection = 0;
        HighlightRandomizerMenuItem(gTasks[taskId].tMenuSelection);
    }
    else if (JOY_NEW(DPAD_LEFT) || JOY_NEW(DPAD_RIGHT))
    {
        switch (gTasks[taskId].tMenuSelection)
        {
        case MENUITEM_P2_ENCOUNTER_RAND_ENABLED:
            gTasks[taskId].tEncounterRandEnabled = !gTasks[taskId].tEncounterRandEnabled;
            DrawEncounterRandEnabledChoices(gTasks[taskId].tEncounterRandEnabled);
            CopyWindowToVram(WIN_RAND_OPTIONS, COPYWIN_GFX);
            break;
        default:
            return;
        }
    }
}

static void DrawPageItems(u8 taskId)
{
    u8 i;
    u8 count;
    const u8* const* names;

    FillWindowPixelBuffer(WIN_RAND_OPTIONS, PIXEL_FILL(1));

    if (gTasks[taskId].tPage == 0)
    {
        count = MENUITEM_P1_COUNT;
        names = sPage1ItemNames;
    }
    else
    {
        count = MENUITEM_P2_COUNT;
        names = sPage2ItemNames;
    }

    for (i = 0; i < count; i++)
        AddTextPrinterParameterized(WIN_RAND_OPTIONS, FONT_NORMAL,
            names[i], 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);
}

static void DrawPageValues(u8 taskId)
{
    if (gTasks[taskId].tPage == 0)
    {
        DrawStarterRandEnabledChoices(gTasks[taskId].tStarterRandEnabled);
        DrawIncludeLegendariesChoices(gTasks[taskId].tStarterRandIncludeLegendaries, AllOptionsLocked(taskId));
        DrawPoolModeChoices(gTasks[taskId].tStarterRandPoolMode, AllOptionsLocked(taskId));
        DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE1, gTasks[taskId].tStarterRandTypeSlot1, TypeSlotsAreLocked(taskId));
        DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE2, gTasks[taskId].tStarterRandTypeSlot2, TypeSlotsAreLocked(taskId));
        DrawTypeSlotChoices(MENUITEM_P1_STARTER_RAND_TYPE3, gTasks[taskId].tStarterRandTypeSlot3, TypeSlotsAreLocked(taskId));
        // NEXT row has no value to draw
    }
    else
    {
        DrawEncounterRandEnabledChoices(gTasks[taskId].tEncounterRandEnabled);
        // PREV and CANCEL rows have no values to draw
    }
}