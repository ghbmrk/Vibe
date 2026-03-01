#ifndef COMMON_H
#define COMMON_H

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>

/* =========================================================
   CREATURE QUEST - A GBC Monster-Battling RPG
   ========================================================= */

/* --- Game states --- */
#define STATE_TITLE      0
#define STATE_OVERWORLD  1
#define STATE_BATTLE     2
#define STATE_MENU       3
#define STATE_SHOP       4
#define STATE_LEVELUP    5
#define STATE_INTRO      6

/* --- Directions --- */
#define DIR_UP    0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_RIGHT 3

/* --- Types (element system) --- */
#define TYPE_NORMAL  0
#define TYPE_FIRE    1
#define TYPE_WATER   2
#define TYPE_LEAF    3
#define TYPE_BOLT    4
#define TYPE_STONE   5
#define TYPE_SHADOW  6
#define TYPE_LIGHT   7
#define NUM_TYPES    8

/* --- Move categories --- */
#define CAT_PHYSICAL 0
#define CAT_SPECIAL  1
#define CAT_STATUS   2

/* --- Status effects --- */
#define STATUS_NONE    0
#define STATUS_BURN    1
#define STATUS_POISON  2
#define STATUS_PARALYZE 3
#define STATUS_SLEEP   4

/* --- Items --- */
#define ITEM_NONE      0
#define ITEM_POTION    1
#define ITEM_ELIXIR    2
#define ITEM_REVIVE    3
#define ITEM_ANTIDOTE  4
#define ITEM_SMOKEBALL 5
#define NUM_ITEM_TYPES 6

/* --- Constants --- */
#define MAX_PARTY      3
#define MAX_MOVES      4
#define MAX_LEVEL      50
#define MAX_ITEMS      20
#define NUM_SPECIES    12
#define NUM_ZONES      6

#define MAP_W          20
#define MAP_H          18

#define MOVES_PER_SPECIES 6

/* --- Tile indices for VRAM --- */
#define TILE_BLANK     0
#define TILE_FONT_START 1
/* Font: 1-70 (A-Z, a-z, 0-9, punctuation, space) */
#define TILE_UI_START  72
/* UI: 72-87 (borders, HP bar, menu cursor, etc) */
#define TILE_TERRAIN_START 88
/* Terrain: 88-103 (grass, water, tree, path, etc) */
#define TILE_CREATURE_START 0
/* Creature sprites use OBJ tiles 0-47 (4 tiles per creature front) */

/* --- Sprite OAM slots --- */
#define SPR_PLAYER     0
#define SPR_CREATURE   4

/* --- Move definition --- */
typedef struct {
    const char *name;
    uint8_t type;
    uint8_t category;  /* physical / special / status */
    uint8_t power;
    uint8_t accuracy;  /* out of 100 */
    uint8_t pp_max;
    uint8_t effect;    /* status effect to apply, or 0 */
    uint8_t effect_chance; /* % chance of effect */
} MoveData;

/* --- Species definition --- */
typedef struct {
    const char *name;
    uint8_t type1;
    uint8_t type2;     /* same as type1 if mono-type */
    uint8_t base_hp;
    uint8_t base_atk;
    uint8_t base_def;
    uint8_t base_spatk;
    uint8_t base_spdef;
    uint8_t base_speed;
    uint8_t learn_moves[MOVES_PER_SPECIES]; /* move indices learned */
    uint8_t learn_levels[MOVES_PER_SPECIES]; /* levels they're learned at */
    uint8_t evolve_into; /* species index, 0xFF = no evolution */
    uint8_t evolve_level;
    uint8_t sprite_id;
} SpeciesData;

/* --- Instance of a creature (in party) --- */
typedef struct {
    uint8_t species;
    uint8_t level;
    uint16_t exp;
    uint16_t hp;
    uint16_t max_hp;
    uint8_t atk, def, spatk, spdef, speed;
    uint8_t moves[MAX_MOVES];
    uint8_t pp[MAX_MOVES];
    uint8_t status;
    uint8_t status_turns;
} Creature;

/* --- Battle state for one side --- */
typedef struct {
    Creature *mon;
    int16_t atk_stage;
    int16_t def_stage;
    int16_t spatk_stage;
    int16_t spdef_stage;
    int16_t speed_stage;
    uint8_t is_defending;
} BattleSide;

/* --- Zone definition --- */
typedef struct {
    const char *name;
    uint8_t wild_species[4];
    uint8_t wild_min_level;
    uint8_t wild_max_level;
    uint8_t terrain_palette;
    uint8_t has_shop;
    uint8_t boss_species;
    uint8_t boss_level;
} ZoneData;

/* --- Player / save data --- */
typedef struct {
    uint8_t magic;  /* 0xCE = valid save */
    char name[8];
    Creature party[MAX_PARTY];
    uint8_t party_count;
    uint8_t current_zone;
    uint8_t zones_cleared;
    uint16_t gold;
    uint8_t items[NUM_ITEM_TYPES];
    uint8_t badges;
    uint8_t player_x;
    uint8_t player_y;
    uint8_t player_dir;
} SaveData;

/* --- Globals (extern) --- */
extern SaveData save;
extern uint8_t game_state;
extern uint8_t jpad;
extern uint8_t jpad_prev;
extern uint8_t frame_count;

/* --- Utility macros --- */
#define J_PRESSED(btn) ((jpad & (btn)) && !(jpad_prev & (btn)))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#endif /* COMMON_H */
