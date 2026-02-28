#ifndef COMMON_H
#define COMMON_H

/*  common.h  –  Shared types, constants and macros
 *  for the Creature Collector GBC game.
 */

#include <gb/gb.h>
#include <stdint.h>

/* ======== Game states (main state machine) ================ */
#define STATE_TITLE       0
#define STATE_STARTER     1
#define STATE_OVERWORLD   2
#define STATE_BATTLE      3
#define STATE_MENU        4
#define STATE_SKILLTREE   5

/* ======== Creature element types ========================== */
#define TYPE_FLAME   0
#define TYPE_AQUA    1
#define TYPE_TERRA   2
#define TYPE_VOLT    3
#define TYPE_SHADOW  4
#define TYPE_AETHER  5
#define NUM_TYPES    6

/* ======== Skill categories ================================ */
#define SKILL_ATTACK   0
#define SKILL_DEFEND   1
#define SKILL_SUPPORT  2
#define SKILL_SPECIAL  3
#define NUM_SKILL_CATS 4

/* ======== Battle sub-states =============================== */
#define BSTATE_INIT           0
#define BSTATE_PLAYER_MENU    1
#define BSTATE_SELECT_SKILL   2
#define BSTATE_PLAYER_ACT     3
#define BSTATE_ENEMY_ACT      4
#define BSTATE_CHECK          5
#define BSTATE_VICTORY        6
#define BSTATE_DEFEAT         7
#define BSTATE_CATCH_TRY      8
#define BSTATE_RUN            9
#define BSTATE_DONE          10

/* ======== Limits ========================================== */
#define MAX_PARTY        4
#define MAX_SKILL_NODES  16
#define MAX_ACTIVE_SKILLS 8
#define MAX_SPECIES      6
#define MAX_LEVEL        50
#define NAME_LEN         9

/* ======== Map dimensions (per screen) ===================== */
#define MAP_W  20
#define MAP_H  18

/* ======== Tile index layout in VRAM ======================= */
/*  0        = blank / space                                   */
/*  1  - 26  = font  A-Z                                       */
/*  27 - 36  = font  0-9                                       */
/*  37 - 44  = font  punctuation  ! ? . - / : ( )              */
/*  48 - 63  = overworld tiles                                 */
/*  64 - 79  = UI tiles (box borders, HP bar, etc.)            */
/*  80 - 95  = skill-tree tiles                                */
/*  128-255  = creature battle sprites (loaded per-battle)     */

#define TILE_BLANK       0

/* Font range */
#define TILE_FONT_START  1
#define TILE_FONT_A      1
#define TILE_FONT_0      27
#define TILE_FONT_BANG   37
#define TILE_FONT_QMARK  38
#define TILE_FONT_DOT    39
#define TILE_FONT_DASH   40
#define TILE_FONT_SLASH  41
#define TILE_FONT_COLON  42
#define TILE_FONT_LPAREN 43
#define TILE_FONT_RPAREN 44
#define FONT_TILE_COUNT  45      /* tiles 0..44 */

/* Overworld tiles */
#define TILE_GRASS       48
#define TILE_TALLGRASS   49
#define TILE_PATH        50
#define TILE_WATER       51
#define TILE_TREE_TL     52
#define TILE_TREE_TR     53
#define TILE_TREE_BL     54
#define TILE_TREE_BR     55
#define TILE_ROCK        56
#define TILE_FENCE_H     57
#define TILE_FENCE_V     58
#define TILE_DOOR        59
#define TILE_ROOF        60
#define TILE_WALL        61
#define TILE_FLOWER      62
#define TILE_SIGN        63
#define OW_TILE_COUNT    16       /* 48..63 */

/* UI tiles */
#define TILE_BOX_TL      64
#define TILE_BOX_T       65
#define TILE_BOX_TR      66
#define TILE_BOX_L       67
#define TILE_BOX_MID     68
#define TILE_BOX_R       69
#define TILE_BOX_BL      70
#define TILE_BOX_B       71
#define TILE_BOX_BR      72
#define TILE_HP_FULL     73
#define TILE_HP_MID      74
#define TILE_HP_EMPTY    75
#define TILE_ARROW_R     76
#define TILE_ARROW_D     77
#define TILE_SEL_L       78
#define TILE_SEL_R       79
#define UI_TILE_COUNT    16       /* 64..79 */

/* Skill-tree tiles */
#define TILE_NODE_LOCK   80
#define TILE_NODE_OPEN   81
#define TILE_NODE_SEL    82
#define TILE_LINE_H      83
#define TILE_LINE_V      84
#define TILE_BRANCH_DL   85
#define TILE_BRANCH_DR   86
#define TILE_NODE_ROOT   87
#define ST_TILE_COUNT     8       /* 80..87 */

/* Creature battle sprites – loaded dynamically */
#define TILE_CREA_BASE  128
#define CREA_SPRITE_TILES 16     /* 4x4 tiles = 32x32 px */

/* OAM sprite indices */
#define SPR_PLAYER_0     0
#define SPR_PLAYER_1     1
#define SPR_PLAYER_2     2
#define SPR_PLAYER_3     3
#define SPR_CURSOR       4

/* Player sprite tile in sprite VRAM */
#define STILE_PLAYER     0       /* tiles 0-3 */

/* ======== Directions ====================================== */
#define DIR_DOWN   0
#define DIR_UP     1
#define DIR_LEFT   2
#define DIR_RIGHT  3

/* ======== Data structures ================================= */

/* Skill node in a creature's procedural skill tree */
typedef struct {
    uint8_t id;          /* 0 .. MAX_SKILL_NODES-1              */
    uint8_t parent;      /* parent index, 0xFF = root           */
    uint8_t category;    /* SKILL_ATTACK / DEFEND / SUPPORT / SPECIAL */
    uint8_t element;     /* TYPE_FLAME … TYPE_AETHER            */
    uint8_t power;       /* base power  1-99                    */
    uint8_t cost;        /* SP cost in battle                   */
    uint8_t req_level;   /* level required to unlock            */
    uint8_t flags;       /* bit 0 = unlocked                    */
} SkillNode;

/* Complete skill tree */
typedef struct {
    SkillNode nodes[MAX_SKILL_NODES];
    uint8_t   count;     /* number of nodes actually used       */
} SkillTree;

/* A creature instance (party member or wild encounter) */
typedef struct {
    uint8_t   species;   /* 0..MAX_SPECIES-1                    */
    uint8_t   type;      /* element type                        */
    uint8_t   level;
    uint16_t  hp;
    uint16_t  max_hp;
    uint8_t   atk;
    uint8_t   def;
    uint8_t   spd;
    uint8_t   spc;       /* special stat                        */
    uint16_t  exp;
    uint16_t  exp_next;
    uint8_t   skill_pts; /* unspent skill points                */
    uint8_t   sp;        /* battle SP (spirit points)           */
    uint8_t   sp_max;
    uint16_t  tree_seed; /* deterministic seed for the tree     */
    SkillTree tree;
    char      name[NAME_LEN];
} Creature;

/* ROM-resident species base data */
typedef struct {
    char    name[NAME_LEN];
    uint8_t type;
    uint8_t base_hp;
    uint8_t base_atk;
    uint8_t base_def;
    uint8_t base_spd;
    uint8_t base_spc;
    uint8_t catch_rate; /* 1-255, higher = easier to catch */
} SpeciesData;

/* SRAM save structure */
typedef struct {
    uint8_t   magic;          /* 0xCC = valid save                 */
    uint8_t   party_count;
    Creature  party[MAX_PARTY];
    uint8_t   current_zone;
    uint8_t   in_gym;
    uint8_t   boss_beaten;
    uint8_t   player_x;
    uint8_t   player_y;
    uint8_t   player_dir;
    uint16_t  battles_won;
    uint8_t   catches;
    uint8_t   checksum;
} SaveData;

/* ======== Global game state (defined in main.c) =========== */
extern uint8_t  game_state;
extern uint8_t  party_count;
extern Creature party[MAX_PARTY];
extern uint8_t  current_zone;
extern uint8_t  in_gym;
extern uint8_t  boss_beaten;
extern uint8_t  player_x;
extern uint8_t  player_y;
extern uint8_t  player_dir;
extern uint16_t battles_won;
extern uint8_t  total_catches;
extern uint8_t  jpad;          /* current frame joypad state */
extern uint8_t  jpad_prev;     /* previous frame joypad state */

/* ======== Utility macros ================================== */
#define NODE_UNLOCKED(n)  ((n).flags & 0x01u)
#define NODE_UNLOCK(n)    ((n).flags |= 0x01u)
#define NODE_LOCK(n)      ((n).flags &= (uint8_t)~0x01u)

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* Detect a fresh button press (went from 0→1 this frame) */
#define PRESSED(key)  ((jpad & (key)) && !(jpad_prev & (key)))

#define SAVE_MAGIC 0xCC

#endif /* COMMON_H */
