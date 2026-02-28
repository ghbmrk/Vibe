/*  skilltree.c  –  Procedural skill-tree generation with keystones.
 *
 *  Each creature gets a unique skill tree determined by its species
 *  element type and a 16-bit seed.  The tree is a rooted DAG with
 *  2-3 main branches, each 3-5 nodes deep, with sub-branches up to
 *  2 deep.  Every creature of the same species+seed produces the
 *  same tree, so it is deterministic and saveable.
 *
 *  Each branch contains one keystone node (a build-defining passive
 *  or enhanced-active) and ends with a capstone (high-power skill).
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

/* ---- Keystone pool tables --------------------------------- */

static const uint8_t atk_keystones[] = { NTYPE_VAMPIRIC, NTYPE_BERSERK, NTYPE_GLASS };
static const uint8_t def_keystones[] = { NTYPE_THORNS, NTYPE_FORTRESS, NTYPE_LAST_STAND };
static const uint8_t sup_keystones[] = { NTYPE_REGEN, NTYPE_LEECH_SP };
static const uint8_t spc_keystones[] = { NTYPE_MASTERY, NTYPE_SWIFT, NTYPE_MULTICAST, NTYPE_DRAIN };

static uint8_t pick_keystone(uint8_t branch_cat) {
    switch (branch_cat) {
        case SKILL_ATTACK:  return atk_keystones[t_range(0, 2)];
        case SKILL_DEFEND:  return def_keystones[t_range(0, 2)];
        case SKILL_SUPPORT: return sup_keystones[t_range(0, 1)];
        default:            return spc_keystones[t_range(0, 3)];
    }
}

/* ---- Name builder ----------------------------------------- */

void skilltree_skill_name(char *buf, uint8_t element, uint8_t category,
                          uint8_t variant) {
    const char *pfx;
    const char *sfx;
    uint8_t i = 0, j;

    switch (element) {
        case TYPE_FLAME:  pfx = pfx_flame [variant & 3]; break;
        case TYPE_AQUA:   pfx = pfx_aqua  [variant & 3]; break;
        case TYPE_TERRA:  pfx = pfx_terra [variant & 3]; break;
        case TYPE_VOLT:   pfx = pfx_volt  [variant & 3]; break;
        case TYPE_SHADOW: pfx = pfx_shadow[variant & 3]; break;
        default:          pfx = pfx_aether[variant & 3]; break;
    }
    switch (category) {
        case SKILL_ATTACK:  sfx = sfx_atk[variant & 3]; break;
        case SKILL_DEFEND:  sfx = sfx_def[variant & 3]; break;
        case SKILL_SUPPORT: sfx = sfx_sup[variant & 3]; break;
        default:            sfx = sfx_spc[variant & 3]; break;
    }

    for (j = 0; pfx[j] && i < SKILL_NAME_LEN - 1; j++) buf[i++] = pfx[j];
    if (i < SKILL_NAME_LEN - 1) buf[i++] = ' ';
    for (j = 0; sfx[j] && i < SKILL_NAME_LEN - 1; j++) buf[i++] = sfx[j];
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

const char *skilltree_ntype_name(uint8_t ntype) {
    switch (ntype) {
        case NTYPE_THORNS:     return "THORNS";
        case NTYPE_BERSERK:    return "BERSERK";
        case NTYPE_REGEN:      return "REGEN";
        case NTYPE_GLASS:      return "GLASS";
        case NTYPE_SWIFT:      return "SWIFT";
        case NTYPE_MASTERY:    return "MASTERY";
        case NTYPE_LAST_STAND: return "ENDURE";
        case NTYPE_FORTRESS:   return "FORT";
        case NTYPE_VAMPIRIC:   return "VAMPIRIC";
        case NTYPE_LEECH_SP:   return "LEECH";
        case NTYPE_MULTICAST:  return "MULTI";
        case NTYPE_DRAIN:      return "DRAIN";
        default:               return "";
    }
}

const char *skilltree_ntype_desc(uint8_t ntype) {
    switch (ntype) {
        case NTYPE_THORNS:     return "REFLECT DMG";
        case NTYPE_BERSERK:    return "LOW HP POWER";
        case NTYPE_REGEN:      return "HEAL/TURN";
        case NTYPE_GLASS:      return "ATK UP DEF DN";
        case NTYPE_SWIFT:      return "ACT FIRST";
        case NTYPE_MASTERY:    return "SE BONUS 2X";
        case NTYPE_LAST_STAND: return "SURVIVE ONCE";
        case NTYPE_FORTRESS:   return "DEF PERSISTS";
        case NTYPE_VAMPIRIC:   return "HEAL ON HIT";
        case NTYPE_LEECH_SP:   return "SP ON HIT";
        case NTYPE_MULTICAST:  return "DOUBLE CAST";
        case NTYPE_DRAIN:      return "DRAIN SP";
        default:               return "";
    }
}

/* ---- Tree generation -------------------------------------- */

void skilltree_generate(SkillTree *tree, uint8_t creature_type, uint16_t seed) {
    uint8_t idx, branch, depth, i;
    uint8_t num_branches;
    uint8_t branch_cat[3];
    uint8_t prev;
    uint8_t base_pwr;
    uint8_t ks_depth;
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
    NODE_SET_NTYPE(*n, NTYPE_NORMAL);
    idx = 1;

    /* ---- Decide branches ----------------------------------- */
    num_branches = t_range(2, 3);

    branch_cat[0] = SKILL_ATTACK;
    for (i = 1; i < num_branches; i++) {
        branch_cat[i] = t_range(1, 3);
    }
    /* Ensure no duplicate categories when possible */
    if (num_branches >= 2 && branch_cat[1] == branch_cat[0]) {
        branch_cat[1] = (branch_cat[0] + 1u + t_range(0, 1)) % NUM_SKILL_CATS;
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
        ks_depth = t_range(2, 3); /* depth at which keystone appears */
        if (ks_depth >= depth) ks_depth = depth - 2;

        for (i = 0; i < depth && idx < MAX_SKILL_NODES; i++) {
            uint8_t is_keystone;
            uint8_t is_capstone;

            n = &tree->nodes[idx];
            n->id       = idx;
            n->parent   = prev;
            n->category = branch_cat[branch];
            n->element  = creature_type;
            n->flags    = 0;

            is_keystone = (i == ks_depth) ? 1 : 0;
            is_capstone = (i == depth - 1) ? 1 : 0;

            /* Occasional category variation after first 2 nodes */
            if (!is_keystone && !is_capstone && i > 1 && t_range(0, 3) == 0) {
                n->category = t_range(0, 3);
            }
            /* Occasional off-element skill */
            if (!is_keystone && t_range(0, 5) == 0) {
                n->element = t_range(0, NUM_TYPES - 1);
            }

            /* ---- Keystone node -------------------------------- */
            if (is_keystone) {
                uint8_t ks = pick_keystone(branch_cat[branch]);
                NODE_SET_NTYPE(*n, ks);

                if (ks >= 1 && ks <= 8) {
                    /* Passive: no power/cost */
                    n->power = 0;
                    n->cost  = 0;
                } else {
                    /* Enhanced active */
                    n->power = base_pwr + i * 14u + t_range(4, 12);
                    if (n->power > 99) n->power = 99;
                    n->cost = 2u + n->power / 20u;
                    if (n->cost > 8) n->cost = 8;
                }
            }
            /* ---- Capstone: powerful end-of-branch skill ------- */
            else if (is_capstone) {
                NODE_SET_NTYPE(*n, NTYPE_NORMAL);
                n->power = 70u + t_range(0, 29);
                if (n->power > 99) n->power = 99;
                n->cost = 4u + t_range(0, 3);
                if (n->cost > 8) n->cost = 8;
                /* Capstones often gain off-element for coverage */
                if (t_range(0, 2) == 0) {
                    n->element = t_range(0, NUM_TYPES - 1);
                }
            }
            /* ---- Normal node ---------------------------------- */
            else {
                NODE_SET_NTYPE(*n, NTYPE_NORMAL);
                n->power = base_pwr + i * 14u + t_range(0, 8);
                if (n->power > 99) n->power = 99;
                n->cost = 2u + n->power / 20u + t_range(0, 1);
                if (n->cost > 8) n->cost = 8;
            }

            /* Level requirement scales with depth */
            n->req_level = 3u + i * 4u + t_range(0, 2);
            if (n->req_level > MAX_LEVEL) n->req_level = MAX_LEVEL;

            prev = idx;
            idx++;

            /* ---- Sub-branch (40% chance, 1-2 nodes deep) ------ */
            if (i >= 1 && i < depth - 1 &&
                idx < MAX_SKILL_NODES - 2 &&
                t_range(0, 4) < 2) {
                uint8_t sub_depth = t_range(1, 2);
                uint8_t sub_prev = prev;
                uint8_t s;

                for (s = 0; s < sub_depth && idx < MAX_SKILL_NODES; s++) {
                    SkillNode *sub = &tree->nodes[idx];
                    sub->id       = idx;
                    sub->parent   = sub_prev;
                    sub->category = t_range(0, 3);
                    sub->element  = creature_type;
                    sub->power    = base_pwr + (i + 1u + s) * 14u + t_range(4, 12);
                    if (sub->power > 99) sub->power = 99;
                    sub->cost     = 2u + sub->power / 20u;
                    if (sub->cost > 8) sub->cost = 8;
                    sub->req_level = n->req_level + t_range(2, 4) + s * 3u;
                    if (sub->req_level > MAX_LEVEL) sub->req_level = MAX_LEVEL;
                    sub->flags = 0;
                    NODE_SET_NTYPE(*sub, NTYPE_NORMAL);
                    sub_prev = idx;
                    idx++;
                }
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

    if (NODE_UNLOCKED(*n)) return 0;
    if (creature_level < n->req_level) return 0;
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
        if (NODE_UNLOCKED(tree->nodes[i]) && !NODE_IS_PASSIVE(tree->nodes[i])) {
            out[count++] = i;
        }
    }
    return count;
}

void skilltree_auto_unlock(SkillTree *tree, uint8_t level, uint16_t seed) {
    uint8_t pts, i, count, pick;
    uint8_t candidates[MAX_SKILL_NODES];
    uint16_t rng = seed ? seed : 1u;

    /* Skill points: 1 initial + 1 per 2 levels */
    pts = 1u + level / 2u;

    while (pts > 0) {
        count = 0;
        for (i = 0; i < tree->count; i++) {
            if (skilltree_can_unlock(tree, i, level)) {
                candidates[count++] = i;
            }
        }
        if (count == 0) break;

        rng ^= rng << 7;
        rng ^= rng >> 9;
        rng ^= rng << 8;
        pick = candidates[rng % (uint16_t)count];
        skilltree_unlock(tree, pick);
        pts--;
    }
}
