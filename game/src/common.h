/* common.h - Shared types, constants, macros for IMPERIAL HUNT */
#ifndef COMMON_H
#define COMMON_H

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>

/* ── Screen dimensions ─────────────────────────────────────── */
#define SCREEN_W    20
#define SCREEN_H    18
#define MAP_W       32
#define MAP_H       32

/* ── Tile index assignments ────────────────────────────────── */
/* Terrain (BG tiles 16-31) */
#define TILE_GRASS       16
#define TILE_TALL_GRASS  17
#define TILE_WALL        18
#define TILE_PATH        19
#define TILE_WATER       20
#define TILE_DOOR        21
#define TILE_GYM_FLOOR   22
#define TILE_BOSS_MARK   23
#define TILE_VENDOR      24
#define TILE_ROCK        25

/* UI (BG tiles 64-79) */
#define TILE_BLANK       64
#define TILE_BOX_TL      65
#define TILE_BOX_TR      66
#define TILE_BOX_BL      67
#define TILE_BOX_BR      68
#define TILE_BOX_H       69
#define TILE_BOX_V       70
#define TILE_HP_FULL     71
#define TILE_HP_EMPTY    72
#define TILE_HP_LCAP     73
#define TILE_HP_RCAP     74
#define TILE_CURSOR      75
#define TILE_FILL        76

/* Creature battle sprites (BG tiles 32-63) */
#define TILE_CREATURE1   32   /* player creature 4x4 */
#define TILE_CREATURE2   48   /* enemy creature 4x4 */

/* Font (BG tiles 128-223) */
#define TILE_FONT_BASE   128
/* ' ' = 128, A-Z = 129-154, 0-9 = 155-164, punct = 165+ */

/* Sprite tile indices (OAM) */
#define SPR_TILE_PLAYER_DOWN   0
#define SPR_TILE_PLAYER_UP     4
#define SPR_TILE_PLAYER_SIDE   8

/* ── Element types ─────────────────────────────────────────── */
#define ELEM_FIRE       0
#define ELEM_WATER      1
#define ELEM_EARTH      2
#define ELEM_ELECTRIC   3
#define ELEM_SHADOW     4
#define ELEM_LIGHT      5
#define ELEM_NORMAL     6
#define ELEM_COUNT      7

/* ── Species ───────────────────────────────────────────────── */
#define SP_EMBERON      0
#define SP_TIDALIN      1
#define SP_TERRAVOLT    2
#define SP_ZAPPIX       3
#define SP_SHADRIX      4
#define SP_LUMINOS      5
#define SP_COUNT        6

/* ── Game states ───────────────────────────────────────────── */
#define ST_TITLE        0
#define ST_STARTER      1
#define ST_OVERWORLD    2
#define ST_BATTLE       3
#define ST_SHOP         4
#define ST_SKILLTREE    5
#define ST_GAMEOVER     6
#define ST_MENU         7

/* ── Battle sub-states ─────────────────────────────────────── */
#define BS_INTRO        0
#define BS_MENU         1
#define BS_FIGHT_SEL    2
#define BS_PLAYER_TURN  3
#define BS_ENEMY_TURN   4
#define BS_CHECK_KO     5
#define BS_WIN          6
#define BS_LOSE         7
#define BS_CHOICE       8
#define BS_CATCH        9
#define BS_LEVELUP_CHAR 10
#define BS_LEVELUP_CREA 11
#define BS_RUN          12

/* ── Gear ──────────────────────────────────────────────────── */
#define GEAR_WEAPON     0
#define GEAR_ARMOR      1
#define GEAR_ACCESSORY  2
#define GEAR_SLOTS      3
#define GEAR_NONE       0xFF

/* ── Directions ────────────────────────────────────────────── */
#define DIR_DOWN        0
#define DIR_UP          1
#define DIR_LEFT        2
#define DIR_RIGHT       3

/* ── Max limits ────────────────────────────────────────────── */
#define MAX_MOVES       4
#define MAX_SKILLS      16
#define MAX_LEVEL       50
#define MAX_NAME_LEN    10
#define MAX_GEAR_ITEMS  12

/* ── Skill IDs (character) ─────────────────────────────────── */
#define CSKILL_CHA_UP       0   /* +1 CHA */
#define CSKILL_WIS_UP       1   /* +1 WIS */
#define CSKILL_LCK_UP       2   /* +1 LCK */
#define CSKILL_CON_UP       3   /* +1 CON */
#define CSKILL_IRON_HALO    4   /* creature -10% dmg taken */
#define CSKILL_BATTLE_CRY   5   /* creature +15% ATK 3 turns */
#define CSKILL_TACTICAL     6   /* creature wins speed ties */
#define CSKILL_VETERANS_EYE 7   /* +10% catch rate */
#define CSKILL_BLESSED      8   /* creature ignores 10% DEF */
#define CSKILL_EMPERORS_LT  9   /* +15% XP */
#define CSKILL_WAR_TROPHY   10  /* +20% gold */
#define CSKILL_ARTIFICER    11  /* gear bonuses +50% */
#define CSKILL_COUNT        12

/* ── Skill IDs (creature) ──────────────────────────────────── */
#define MSKILL_ATK_UP       0   /* +2 ATK */
#define MSKILL_DEF_UP       1   /* +2 DEF */
#define MSKILL_SPD_UP       2   /* +2 SPD */
#define MSKILL_HP_UP        3   /* +5 max HP */
#define MSKILL_THICK_HIDE   4   /* -2 flat dmg taken */
#define MSKILL_FURY         5   /* +25% ATK below 50% HP */
#define MSKILL_REGEN        6   /* heal 5% per turn */
#define MSKILL_TYPE_MASTERY 7   /* STAB +25% */
#define MSKILL_EVASION      8   /* 10% dodge */
#define MSKILL_TOUGHNESS    9   /* +10 max HP */
#define MSKILL_COUNTER      10  /* reflect 20% melee */
#define MSKILL_QUICK_STRIKE 11  /* +3 SPD */
#define MSKILL_COUNT        12

/* ── Move IDs ──────────────────────────────────────────────── */
#define MOVE_TACKLE      0
#define MOVE_EMBER        1
#define MOVE_SPLASH       2
#define MOVE_ROCKFALL     3
#define MOVE_SPARK        4
#define MOVE_HEX          5
#define MOVE_FLASH        6
#define MOVE_BLAZE        7
#define MOVE_TORRENT      8
#define MOVE_QUAKE        9
#define MOVE_THUNDER     10
#define MOVE_VOID        11
#define MOVE_RADIANCE    12
#define MOVE_INFERNO     13
#define MOVE_TSUNAMI     14
#define MOVE_TREMOR      15
#define MOVE_SURGE       16
#define MOVE_ECLIPSE     17
#define MOVE_NOVA        18
#define MOVE_STRIKE      19
#define MOVE_SLAM        20
#define MOVE_RUSH        21
#define MOVE_COUNT       22

/* ── Data structures ───────────────────────────────────────── */

typedef struct {
    uint8_t type;       /* ELEM_ */
    uint8_t power;
    uint8_t accuracy;   /* 0-100 */
    uint8_t sp_cost;
} MoveData;

typedef struct {
    uint8_t hp, atk, def, spd;
    uint8_t type;
    uint8_t start_move1, start_move2;
} SpeciesData;

typedef struct {
    uint8_t level;
    uint16_t xp;
    uint16_t xp_next;
    uint8_t cha, wis, lck, con;
    uint8_t gear[GEAR_SLOTS];  /* gear IDs or GEAR_NONE */
    uint16_t gold;
    uint8_t skills[MAX_SKILLS];
    uint8_t num_skills;
} Character;

typedef struct {
    uint8_t species;
    uint8_t level;
    uint16_t xp;
    uint16_t xp_next;
    uint8_t hp, max_hp;
    uint8_t sp, max_sp;
    uint8_t atk, def, spd;
    uint8_t type;
    uint8_t gear[GEAR_SLOTS];
    uint8_t moves[MAX_MOVES];
    uint8_t num_moves;
    uint8_t skills[MAX_SKILLS];
    uint8_t num_skills;
} Creature;

typedef struct {
    char name[MAX_NAME_LEN + 1];
    uint16_t cost;
    int8_t  bonus;       /* stat bonus value */
    uint8_t slot;        /* GEAR_WEAPON/ARMOR/ACCESSORY */
    uint8_t for_creature;/* 0=character, 1=creature */
} GearDef;

typedef struct {
    uint8_t zone_num;
    uint8_t theme;
    uint8_t map[MAP_H][MAP_W];
    uint8_t gym_map[SCREEN_H][SCREEN_W];
    uint8_t entry_x, entry_y;
    uint8_t gym_x, gym_y;
    uint8_t boss_x, boss_y;
    uint8_t base_level;
    uint8_t boss_defeated;
} ZoneData;

typedef struct {
    uint8_t state;
    Character player;
    Creature creature;
    ZoneData zone;
    uint8_t px, py;          /* player position (tile coords) */
    uint8_t dir;             /* facing direction */
    uint8_t in_gym;
    uint16_t battles_won;
    uint16_t creatures_caught;
    uint16_t rng_seed;
} GameData;

/* ── Global game data (defined in main.c) ──────────────────── */
extern GameData game;

/* ── Palette indices ───────────────────────────────────────── */
#define PAL_UI          0
#define PAL_GRASS       1
#define PAL_TREE        2
#define PAL_PATH        3
#define PAL_WATER       4
#define PAL_GYM         5
#define PAL_FIRE        6
#define PAL_ACCENT      7

/* ── Utility macros ────────────────────────────────────────── */
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define ABS(a)   ((a)<0?-(a):(a))

#endif /* COMMON_H */
