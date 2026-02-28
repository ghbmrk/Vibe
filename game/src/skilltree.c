/*  skilltree.c  –  Procedural skill-tree generation.
 *
 *  Each creature gets a unique skill tree determined by its species
 *  element type and a 16-bit seed.  The tree is a rooted DAG with
 *  2-3 main branches, each 3-5 nodes deep, with occasional sub-
 *  branches.  Every creature of the same species+seed produces the
 *  same tree, so it is deterministic and saveable (only the seed and
 *  unlock flags need to persist).
 */

#include "skilltree.h"
#include <string.h>

/* ---- Skill name fragments (ROM) -------------------------- */

static const char pfx_flame[][7]  = { "Blaze",  "Scorch", "Ember",  "Pyro"  };
static const char pfx_aqua[][7]   = { "Tide",   "Splash", "Aqua",   "Hydro" };
static const char pfx_terra[][7]  = { "Rock",   "Quake",  "Stone",  "Geo"   };
static const char pfx_volt[][7]   = { "Spark",  "Zap",    "Bolt",   "Volt"  };
static const char pfx_shadow[][7] = { "Dark",   "Void",   "Night",  "Shade" };
static const char pfx_aether[][7] = { "Star",   "Holy",   "Lux",    "Aura"  };

static const char sfx_atk[][6] = { "Rush",  "Fang",  "Claw",  "Edge"  };
static const char sfx_def[][6] = { "Guard", "Shell", "Wall",  "Block" };
static const char sfx_sup[][6] = { "Heal",  "Mend",  "Balm",  "Cure"  };
static const char sfx_spc[][6] = { "Storm", "Wave",  "Burst", "Nova"  };

/* ---- Local deterministic PRNG (separate from game RNG) ---- */

static uint16_t t_rng;

static uint16_t t_rand(void) {
    t_rng ^= t_rng << 7;
    t_rng ^= t_rng >> 9;
    t_rng ^= t_rng << 8;
    return t_rng;
}

static uint8_t t_range(uint8_t lo, uint8_t hi) {
    if (lo >= hi) return lo;
    return lo + (uint8_t)(t_rand() % (uint16_t)(hi - lo + 1u));
}

/* ---- Name builder ----------------------------------------- */

void skilltree_skill_name(char *buf, uint8_t element, uint8_t category,
                          uint8_t variant) {
    const char *pfx;
    const char *sfx;
    uint8_t i = 0, j;

    /* Choose prefix by element */
    switch (element) {
        case TYPE_FLAME:  pfx = pfx_flame [variant & 3]; break;
        case TYPE_AQUA:   pfx = pfx_aqua  [variant & 3]; break;
        case TYPE_TERRA:  pfx = pfx_terra [variant & 3]; break;
        case TYPE_VOLT:   pfx = pfx_volt  [variant & 3]; break;
        case TYPE_SHADOW: pfx = pfx_shadow[variant & 3]; break;
        default:          pfx = pfx_aether[variant & 3]; break;
    }
    /* Choose suffix by category */
    switch (category) {
        case SKILL_ATTACK:  sfx = sfx_atk[variant & 3]; break;
        case SKILL_DEFEND:  sfx = sfx_def[variant & 3]; break;
        case SKILL_SUPPORT: sfx = sfx_sup[variant & 3]; break;
        default:            sfx = sfx_spc[variant & 3]; break;
    }

    /* Copy prefix (max 6 chars) */
    for (j = 0; pfx[j] && i < NAME_LEN - 1; j++) buf[i++] = pfx[j];
    /* Copy suffix (whatever fits in 9 chars total) */
    for (j = 0; sfx[j] && i < NAME_LEN - 1; j++) buf[i++] = sfx[j];
    buf[i] = '\0';
}

const char *skilltree_cat_tag(uint8_t category) {
    switch (category) {
        case SKILL_ATTACK:  return "ATK";
        case SKILL_DEFEND:  return "DEF";
        case SKILL_SUPPORT: return "SUP";
        default:            return "SPC";
    }
}

/* ---- Tree generation -------------------------------------- */

void skilltree_generate(SkillTree *tree, uint8_t creature_type, uint16_t seed) {
    uint8_t idx, branch, depth, i;
    uint8_t num_branches;
    uint8_t branch_cat[3];
    uint8_t prev;
    uint8_t base_pwr;
    SkillNode *n;

    memset(tree, 0, sizeof(SkillTree));
    t_rng = seed ? seed : 1u;

    /* ---- Node 0: root – basic attack of creature's type ---- */
    n = &tree->nodes[0];
    n->id        = 0;
    n->parent    = 0xFF;
    n->category  = SKILL_ATTACK;
    n->element   = creature_type;
    n->power     = 20;
    n->cost      = 1;
    n->req_level = 1;
    n->flags     = 0;
    idx = 1;

    /* ---- Decide branches ----------------------------------- */
    num_branches = t_range(2, 3);

    /* First branch is always attack; others are random */
    branch_cat[0] = SKILL_ATTACK;
    for (i = 1; i < num_branches; i++) {
        branch_cat[i] = t_range(1, 3); /* DEF / SUP / SPC */
    }
    /* Shuffle first two for variety */
    if (num_branches >= 2 && (t_rand() & 1u)) {
        uint8_t tmp  = branch_cat[0];
        branch_cat[0] = branch_cat[1];
        branch_cat[1] = tmp;
    }

    /* ---- Build each branch --------------------------------- */
    for (branch = 0; branch < num_branches && idx < MAX_SKILL_NODES; branch++) {
        depth    = t_range(3, 5);
        prev     = 0;            /* parent of first node = root */
        base_pwr = 25u + branch * 8u;

        for (i = 0; i < depth && idx < MAX_SKILL_NODES; i++) {
            n = &tree->nodes[idx];
            n->id       = idx;
            n->parent   = prev;
            n->category = branch_cat[branch];
            n->element  = creature_type;

            /* Occasional category variation */
            if (i > 1 && t_range(0, 3) == 0) {
                n->category = t_range(0, 3);
            }
            /* Occasional off-element skill */
            if (t_range(0, 5) == 0) {
                n->element = t_range(0, NUM_TYPES - 1);
            }

            /* Power scales with depth */
            n->power = base_pwr + i * 14u + t_range(0, 8);
            if (n->power > 99) n->power = 99;

            /* Cost scales roughly with power */
            n->cost = 2u + n->power / 20u + t_range(0, 1);
            if (n->cost > 8) n->cost = 8;

            /* Level requirement increases down the branch */
            n->req_level = 3u + i * 5u + t_range(0, 2);
            if (n->req_level > MAX_LEVEL) n->req_level = MAX_LEVEL;

            n->flags = 0;

            prev = idx;
            idx++;

            /* ---- Chance of a sub-branch off this node ----- */
            if (i >= 1 && i < depth - 1 &&
                idx < MAX_SKILL_NODES - 1 &&
                t_range(0, 2) == 0) {
                SkillNode *sub = &tree->nodes[idx];
                sub->id       = idx;
                sub->parent   = prev;
                sub->category = t_range(0, 3);
                sub->element  = creature_type;
                sub->power    = base_pwr + (i + 1u) * 14u + t_range(4, 12);
                if (sub->power > 99) sub->power = 99;
                sub->cost     = 2u + sub->power / 20u;
                if (sub->cost > 8) sub->cost = 8;
                sub->req_level = n->req_level + t_range(2, 4);
                if (sub->req_level > MAX_LEVEL) sub->req_level = MAX_LEVEL;
                sub->flags = 0;
                idx++;
            }
        }
    }

    tree->count = idx;
}

/* ---- Unlock helpers --------------------------------------- */

uint8_t skilltree_can_unlock(const SkillTree *tree, uint8_t node_idx,
                             uint8_t creature_level) {
    const SkillNode *n;
    if (node_idx >= tree->count) return 0;

    n = &tree->nodes[node_idx];

    /* Already unlocked? */
    if (NODE_UNLOCKED(*n)) return 0;

    /* Level gate */
    if (creature_level < n->req_level) return 0;

    /* Parent must be unlocked (root has parent = 0xFF) */
    if (n->parent != 0xFF && !NODE_UNLOCKED(tree->nodes[n->parent])) return 0;

    return 1;
}

void skilltree_unlock(SkillTree *tree, uint8_t node_idx) {
    if (node_idx < tree->count) {
        NODE_UNLOCK(tree->nodes[node_idx]);
    }
}

uint8_t skilltree_get_usable(const SkillTree *tree, uint8_t *out, uint8_t max) {
    uint8_t count = 0;
    uint8_t i;
    for (i = 0; i < tree->count && count < max; i++) {
        if (NODE_UNLOCKED(tree->nodes[i])) {
            out[count++] = i;
        }
    }
    return count;
}
