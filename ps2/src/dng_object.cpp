#include "common.h"
#include "dng_object.hpp"

#include <cmath>
#include <cstdio>
#include <libvu0.h>

#include "character.hpp"
#include "colprim.hpp"
#include "dng_effect.hpp"
#include "dng_event.hpp"
#include "dng_main.hpp"
#include "effscript.hpp"
#include "gamedata.hpp"
#include "gameutil.hpp"
#include "mainloop.hpp"
#include "maintex.hpp"
#include "map.hpp"
#include "mg_camera.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "monster.hpp"
#include "prespr.hpp"
#include "scene.hpp"
#include "scenesnd.hpp"
#include "snd_mngr.hpp"
#include "sound.hpp"
#include "userdata.hpp"

#ifdef NONMATCHING
/**
 * Japanese names of monster badge families.
 */
static char *mons_attr_tbl[12] = {
    "",
    "\203P\203\202\203m",
    "\203J\203\211\203N\203\212",
    "\220\205\220\266\216\355",
    "\220A\225\250",
    "\226\202\226@\220\266\225\250",
    "\226\202\220l",
    "\227\263\221\260",
    "\220\270\227\354",
    "\225s\216\200",
    "\203g\203\211\203\223\203v",
    NULL,
};

/**
 * English names of monster badge families.
 */
static char *mons_attr_tbl2[12] = {
    "",
    "Beast",
    "Windup",
    "Aquatic",
    "Flora",
    "Magical Creature",
    "Darkling",
    "Reptile Family",
    "Spirit",
    "Undead",
    "Card",
    NULL,
};

/**
 * French names of monster badge families.
 */
static char *mons_attr_tbl3[12] = {
    "",
    "Beast",
    "Animal",
    "Robotis[UNI00e9]",
    "Aquatique",
    "V[UNI00e9]g[UNI00e9]tal",
    "Cr[UNI00e9]ature magique",
    "Cr[UNI00e9]ature obscure",
    "Reptile",
    "Esprit",
    "Mort-vivant",
    "Carte",
};

/**
 * German names of monster badge families.
 */
static char *mons_attr_tbl4[12] = {
    "",
    "Tier",
    "Aufzieh-Figur",
    "Wassertier",
    "Flora",
    "Zauberwesen",
    "D[UNI00fc]sterling",
    "Reptil",
    "Geist",
    "Untoter",
    "Karte",
    NULL,
};

/**
 * Italian names of monster badge families.
 */
static char *mons_attr_tbl5[12] = {
    "",
    "Bestia",
    "Robot",
    "Acquatico",
    "Flora",
    "Creatura magica",
    "Oscuro",
    "Rettile",
    "Spirito",
    "Nonmorto",
    "Carta",
    NULL,
};

/**
 * Spanish names of monster badge families.
 */
static char *mons_attr_tbl6[12] = {
    "",
    "Bestia",
    "Broma",
    "Acu[UNI00e1]tico",
    "Flora",
    "Criatura m[UNI00e1]gica",
    "Misterioso",
    "Familia de reptiles",
    "Esp[UNI00ed]ritu",
    "Muerto Viviente",
    "Carta",
    NULL,
};

/**
 * Messages displayed when a monster badge is already owned.
 */
static char *dung_progtxt_badge_already[8] = {
    "\203\202\203\223\203X\203^\201[\202\326\202\361\202\260\203o\203b\203W\201i%s\201j\202\360\n\214\251\202\302\202\257\202\275\201A\202\265\202\251\202\265\202\267\202\305\202\311\223\374\216\350\202\265\202\304\202\242\202\351",
    "You got a Monster Transformation\201i%s\201jBadge,\nbut you already have it.",
    "Tu as re[UNI00e7]u un [UNI00e9]cusson de monstromorphisme (%s),\nmais tu l'as d[UNI00e9]j[UNI00e0].",
    "%s-Monsterwandel-Marke erhalten ,\naber du hast schon einen.",
    "Hai ottenuto uno Stemma Mutazione in (%s) Mostro,\nma ce l\201fhai gi[UNI00e0].",
    "Has conseguido una Insignia de monstruomorfosis(%s),\npero ya la ten[UNI00ed]as.",
    "You got a Monster Transformation (%s) Badge,\nbut you already have it.",
    NULL,
};

/**
 * Messages displayed when a monster badge is acquired.
 */
static char *dung_progtxt_badge_get[8] = {
    "\203\202\203\223\203X\203^\201[\202\326\202\361\202\260\203o\203b\203W\201i%s\201j\202\360\n\216\350\202\311\223\374\202\352\202\275",
    "You got a Monster Transformation\201i%s\201jBadge.",
    "Tu as re[UNI00e7]u un [UNI00e9]cusson \nde monstromorphisme (%s).",
    "Du hast eine neue \n%s-Monsterwandel-Marke.",
    "Hai ottenuto uno Stemma Mutazione\nin (%s) Mostro.",
    "Has conseguido una Insignia de monstruomorfosis (%s).",
    "You got a Monster Transformation (%s)Badge.",
    NULL,
};

/**
 * Messages displayed when the gate key is acquired.
 */
static char *dung_progtxt_gkey_get[8] = {
    "\203Q\201[\203g\203L\201[\201u%s\201v\202\360\216\350\202\311\223\374\202\352\202\275",
    "You got the '%s'.",
    "Tu as trouv[UNI00e9] '%s'.",
    "'%s' erhalten.",
    "Hai trovato '%s'.",
    "Has conseguido '%s'.",
    "You got the '%s'.",
    NULL,
};

/**
 * Messages displayed when a stolen item is recovered.
 */
static char *dung_progtxt_steal[8] = {
    "\223G\202\251\202\347\201u%s\201v\202\360\223\220\202\335\216\346\202\301\202\275",
    "Stolen '%s' from the enemy.",
    "A d[UNI00e9]rob[UNI00e9] '%s' [UNI00e0] l'ennemi.",
    "'%s' erhalten.",
    "Hai rubato '%s' al nemico.",
    "Has robado '%s' al enemigo.",
    "Stolen '%s' from the enemy.",
    NULL,
};

/**
 * Singular and plural messages displayed when an item cannot fit.
 */
static char *dung_progtxt_getitem_overnum[8][2] = {
    {"\201w%s\201x\202\252\223\374\202\301\202\304\202\242\202\351\201B\n\202\265\202\251\202\265\202\261\202\352\210\310\217\343\201A\202\261\202\314\203A\203C\203e\203\200\202\360\216\235\202\302\202\261\202\306\202\252\202\305\202\253\202\310\202\242\201B", "\201w%s\201x\202\252%d\214\302\201A\223\374\202\301\202\304\202\242\202\351\201B\n\202\265\202\251\202\265\202\261\202\352\210\310\217\343\201A\202\261\202\314\203A\203C\203e\203\200\202\360\216\235\202\302\202\261\202\306\202\252\202\305\202\253\202\310\202\242\201B"},
    {"%s inside. \n But you can't carry any more items!", "%d %s inside. \n But you can't carry any more items!"},
    {"Contient %s. \n Mais tu ne peux rien porter d'autre !", "Contient %d %s. \n Mais tu ne peux rien porter d'autre !"},
    {"%s enthalten. \nAber du kannst nicht mehr davon tragen!", "%d %s enthalten. \nAber du kannst nicht mehr davon tragen!"},
    {"%s caricato. \nMa non puoi portare con te altri oggetti!", "%d %s caricato. \nMa non puoi portare con te altri oggetti!"},
    {"Hay %s. \n [UNI00a1]Pero no puedes llevar m[UNI00e1]s objetos!", "Hay %d %s. \n [UNI00a1]Pero no puedes llevar m[UNI00e1]s objetos!"},
    {"%s inside. \n But you can't carry any more items!", "%d %s inside. \n But you can't carry any more items!"},
    {NULL, NULL},
};

/**
 * Singular and plural messages displayed when an item is acquired.
 */
static char *dung_progtxt_getitem[8][2] = {
    {"\201w%s\201x\202\360%d\214\302\201A\216\350\202\311\223\374\202\352\202\275\201B", "\201w%s\201x\202\360%d\214\302\201A\216\350\202\311\223\374\202\352\202\275\201B"},
    {"You found %s.", "You found %d %s."},
    {"Tu as trouv[UNI00e9] %s.", "Tu as trouv[UNI00e9] %d %s."},
    {"%s gefunden.", "%d %s gefunden."},
    {"Hai trovato %s.", "Hai trovato %d %s."},
    {"Has encontrado %s.", "Has encontrado %d %s."},
    {"You found %s.", "You found %d %s."},
    {NULL, NULL},
};

/**
 * Monster badge family names selected by the current language.
 */
static char **mons_attr_list[8] = {
    mons_attr_tbl,
    mons_attr_tbl2,
    mons_attr_tbl3,
    mons_attr_tbl4,
    mons_attr_tbl5,
    mons_attr_tbl6,
    mons_attr_tbl6,
    mons_attr_tbl6,
};
#endif

// Code (.text)
void CRocketLauncher::SetPos(float *start, float *target, float *direction) {
    int i;

    Initialize();
    sceVu0CopyVector(start_pos, start);
    sceVu0CopyVector(pos, start);
    sceVu0CopyVector(target_pos, target);
    sceVu0CopyVector(dir, direction);
    speed = 15.0f;
    state = SHOT_STATE_FIRED;
    trail_timer = 0;
    for (i = 0; i < 16; i++) {
        sceVu0CopyVector(trail[i], pos);
    }
    trail_len = 0;
    homing_delay = 15;
    homing_time = 60;
    life = 150;
    draw_flags = SHOT_DRAW_MODEL | SHOT_DRAW_TRAIL;
}

#ifdef NONMATCHING
void CRocketLauncher::Step() {
    sceVu0FVECTOR    movement;
    sceVu0FVECTOR    old_pos;
    sceVu0FVECTOR    to_target;
    CCPoly          polys[128];
    mgVu0FBOX       box;
    sceVu0FVECTOR    hit_pos;
    sceVu0FVECTOR    effect_pos;
    CColPrim       *prim;
    CCharacter2    *target;
    mgCCamera      *camera;
    CHitEffectImage *hit;
    int             count;

    if (state == SHOT_STATE_FREE) {
        return;
    }
    if (state == SHOT_STATE_FIRED) {
        state = SHOT_STATE_FLYING;
    }
    if (state == SHOT_STATE_FLYING) {
        prim = ColPrimMan.GetID2Prim(col_prim_id);
        if (homing_delay > 0) {
            homing_delay--;
        }
        if (homing_time > 0) {
            homing_time--;
        }
        life--;
        if (homing_delay <= 0 && homing_time > 0) {
            if (target_chara != -1) {
                target = DngMainScene->GetCharacter(target_chara);
                if (target != NULL) {
                    target->GetEntryObjectPos(0, 0, target_pos);
                }
            }
            sceVu0SubVector(to_target, target_pos, pos);
            sceVu0Normalize(to_target, to_target);
            mgVectorInterpolate(dir, dir, to_target, 0.05235988f, 0);
        }
        sceVu0CopyVector(old_pos, pos);
        sceVu0ScaleVector(movement, dir, speed);
        sceVu0AddVector(pos, pos, movement);
        if (prim != NULL) {
            prim->SetCoord(pos, 5.0f);
        }
        box.max[3] = 1.0f;
        box.min[3] = 1.0f;
        box.max[0] = 20.0f + (pos[0] + speed);
        box.min[0] = (pos[0] - speed) - 20.0f;
        box.max[1] = 20.0f + (pos[1] + speed);
        box.min[1] = (pos[1] - speed) - 20.0f;
        box.max[2] = 20.0f + (pos[2] + speed);
        box.min[2] = (pos[2] - speed) - 20.0f;
        count = DngMainMap->GetColPoly(polys, box, 128);
        if (CheckHit(polys, count, pos, old_pos, hit_pos, 1, 4) >= 0) {
            state = SHOT_STATE_BURST;
            draw_flags &= ~SHOT_DRAW_MODEL;
        }
        if (prim != NULL && prim->hit_num > 0) {
            state = SHOT_STATE_BURST;
            draw_flags &= ~SHOT_DRAW_MODEL;
        }
        if (life <= 0) {
            state = SHOT_STATE_BURST;
            draw_flags &= ~SHOT_DRAW_MODEL;
        }
        if (state == SHOT_STATE_BURST) {
            camera = DngMainScene->GetCamera(DngMainScene->active_camera);
            if (camera != NULL) {
                camera->GetPos(effect_pos);
                sceVu0SubVector(effect_pos, effect_pos, pos);
                sceVu0Normalize(effect_pos, effect_pos);
                sceVu0ScaleVector(effect_pos, effect_pos, 20.0f);
                sceVu0AddVector(effect_pos, pos, effect_pos);
            }
            FxScriptMan->CreateEffSpt("\x82\x78\x83\x4f\x83\x8c\x82\x67", 0, -1);
            FxScriptMan->SetScriptVect1(effect_pos, -1, -1);
            sndSePlay(DngMainScene->se_battle_id, 31, 0);
            if (life > 0) {
                sceVu0FVECTOR hit_dir = { 0.0f, 1.0f, 0.0f, 1.0f };

                hit = NULL;
                if (BattleFX.hit != NULL) {
                    hit = &BattleFX.hit[BattleFX.hit_next++];
                    if (BattleFX.hit_next >= BattleFX.hit_num) {
                        BattleFX.hit_next = 0;
                    }
                }
                if (hit != NULL) {
                    hit->SethitEffect(effect_pos, hit_dir, 50.0f, 30.0f, 0.0f, 0.1f, 30, 32);
                    hit->kind = HIT_EFFECT_SPARK_SHORT;
                }
            }
            if (prim != NULL) {
                prim->Delete(-1);
            }
        }
        trail_timer++;
        if (trail_timer >= 3) {
            sceVu0CopyVector(trail[trail_index], pos);
            trail_index++;
            if (trail_index >= 16) {
                trail_index = 0;
            }
            trail_timer = 0;
            trail_len += 5;
        }
    }
    if (state == SHOT_STATE_BURST) {
        trail_len--;
        if (trail_len < 3) {
            state = SHOT_STATE_FREE;
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_object", Step__15CRocketLauncherFv);
#endif

void CRocketLauncher::Draw() {
    if (state == SHOT_STATE_FREE) {
        return;
    }

    CPreSprite    prim;
    sceVu0FVECTOR points[128];
    sceVu0IVECTOR top_left;
    sceVu0IVECTOR bottom_right;
    sceVu0FMATRIX matrix;
    int           count;
    int           i;
    int           color;
    float         alpha;
    float         size;

    if (draw_flags & SHOT_DRAW_TRAIL) {
        count = CreatSmoothPass(points, trail, 16, 6, trail_index, 16);
        if (count < trail_len) {
            trail_len = count;
        }
        prim.Initialize(NULL, NULL);
        prim.Preset2D();
        prim.AlphaBlendEnable(1);
        prim.AlphaTestEnable(1);
        prim.AlphaTest(1, 0);
        prim.DepthTestEnable(1);
        prim.ZMask(-1);
        prim.Bilinear(1);
        prim.TextureMapEnable(1);
        prim.Coord(1);
        prim.Begin(MG_PRIM_SPRITE);
        prim.Texture(trail_texture);
        prim.AlphaTestEnable(1);
        prim.SetAlphaBlend(MG_ALPHA_BLEND_NORMAL);
        prim.Color(128, 128, 128, 128);
        alpha = 1.0f;
        size = 6.0f;
        for (i = count - 1; i >= count - trail_len + 1; i--) {
            points[i][3] = 1.0f;
            if (mgTransWorldPrim3DSprite(top_left, bottom_right, points[i], size, size, 0) != 0) {
                color = (int)(128.0f * alpha);
                prim.Color(color, color, color, color);
                prim.TextureCrd(0, 0);
                prim.Vertex4(top_left);
                prim.TextureCrd(63, 63);
                prim.Vertex4(bottom_right);
            }
            alpha -= 1.0f / trail_len;
            size += 12.0f / trail_len;
        }
        prim.End();
    }
    if (draw_flags & SHOT_DRAW_MODEL) {
        model->SetPosition(pos);
        mgLookAtMatrixZ(matrix, dir);
        model->SetTransMatrix(matrix);
        mgDrawDirect(model);
    }
}

void CRocketLauncher::Initialize(void) {
    target_chara = -1;
    trail_len = 0;
    trail_index = 0;
    state = SHOT_STATE_FREE;
    col_prim_id = -1;
    draw_flags = 0;
}

CRocketLauncher *CRocketLauncherMan::Get() {
    int i;

    for (i = 0; i < 24; i++) {
        if (rocket[i].state == SHOT_STATE_FREE) {
            return &rocket[i];
        }
    }
    return NULL;
}

void CRocketLauncherMan::Draw() {
    mgCTextureManager *textures;
    int                i;

    textures = &mgTexManager;
    for (i = 0; i < 24; i++) {
        textures->ReloadTexture(rocket[i].tex_block, (sceVif1Packet *)NULL);
        rocket[i].Draw();
    }
}

void CRocketLauncherMan::Step() {
    int i;

    for (i = 0; i < 24; i++) {
        rocket[i].Step();
    }
}

void CRocketLauncherMan::Clear() {
    int i;

    for (i = 0; i < 24; i++) {
        rocket[i].Initialize();
    }
}

void CRocketLauncherMan::Initialize(mgCFrame *model, int tex_block, mgCTexture *trail_texture) {
    int             i;
    CRocketLauncher *shot;

    for (i = 0; i < 24; i++) {
        rocket[i].Initialize();
        rocket[i].model = model;
        rocket[i].tex_block = tex_block;
        rocket[i].trail_texture = trail_texture;
    }
}

void CMachineGun::Set(float *start, float *direction) {
    int slot;
    int i;

    slot = -1;
    for (i = 0; i < 16; i++) {
        index++;
        if (index >= 16) {
            index = 0;
        }
        if (active[index] == 0) {
            slot = index;
            break;
        }
    }
    if (slot >= 0) {
        sceVu0CopyVector(start_pos[slot], start);
        sceVu0Normalize(direction, direction);
        sceVu0ScaleVectorXYZ(direction, direction, 40.0f);
        sceVu0CopyVector(velocity[slot], direction);
        sceVu0CopyVector(pos[slot], start);
        active[slot] = 1;
        life[slot] = 90;
    }
}

void CMachineGun::Step() {
    int i;
    for (i = 0; i < 16; i++) {
        s16 state = active[i];
        if (state != 0 && state == 1) {
            CColPrim *prim = ColPrimMan.GetID2Prim(col_prim_id[i]);
            float old_pos[4];
            CCPoly polys[128];
            mgVu0FBOX box;
            float hit_pos[4];

            sceVu0CopyVector(old_pos, pos[i]);
            sceVu0AddVector(pos[i], pos[i], velocity[i]);
            if (prim != NULL) {
                prim->SetCoord(old_pos, pos[i], 5.0f);
            }
            box.max[3] = 1.0f;
            box.min[3] = 1.0f;
            box.max[0] = 20.0f + (40.0f + pos[i][0]);
            box.min[0] = (pos[i][0] - 40.0f) - 20.0f;
            box.max[1] = 20.0f + (40.0f + pos[i][1]);
            box.min[1] = (pos[i][1] - 40.0f) - 20.0f;
            box.max[2] = 20.0f + (40.0f + pos[i][2]);
            box.min[2] = (pos[i][2] - 40.0f) - 20.0f;
            int count = DngMainMap->GetColPoly(polys, box, 128);
            if (CheckHit(polys, count, pos[i], old_pos, hit_pos, 1, 4) >= 0) {
                active[i] = 0;
                if (prim != NULL) {
                    prim->Delete(-1);
                }
                sceVu0FVECTOR hit_dir = {0.0f, 1.0f, 0.0f, 1.0f};
                CHitEffectImage *hit;
                if (BattleFX.hit == NULL) {
                    hit = NULL;
                } else {
                    CHitEffectImage *slot = &BattleFX.hit[BattleFX.hit_next];
                    BattleFX.hit_next++;
                    if (BattleFX.hit_next >= BattleFX.hit_num) {
                        BattleFX.hit_next = 0;
                    }
                    hit = slot;
                }
                if (hit != NULL) {
                    hit->SethitEffect(old_pos, hit_dir, 50.0f, 30.0f, 0.0f, 0.1f, 16, 8);
                    hit->kind = HIT_EFFECT_SPARK_SHORT;
                }
            } else if (prim != NULL && prim->hit_num > 0) {
                active[i] = 0;
                prim->Delete(-1);
            } else {
                life[i]--;
                if (life[i] <= 0) {
                    active[i] = 0;
                    if (prim != NULL) {
                        prim->Delete(-1);
                    }
                }
            }
        }
    }
}

void CLaserGun::SetPos(float *start, float *target, float *direction) {
    int i;

    Initialize();
    sceVu0CopyVector(start_pos, start);
    sceVu0CopyVector(pos, start);
    sceVu0CopyVector(target_pos, target);
    sceVu0CopyVector(dir, direction);
    speed = 20.0f;
    speed_add = 0.0f;
    speed_max = 20.0f;
    state = SHOT_STATE_FIRED;
    trail_timer = 0;
    for (i = 0; i < 8; i++) {
        sceVu0CopyVector(trail[i], pos);
    }
    trail_len = 0;
    homing_delay = 15;
    homing_time = 60;
    life = 120;
    color[0] = 0.0f;
    color[1] = 128.0f;
    color[2] = 128.0f;
    color[3] = 128.0f;
    scale = 1.0f;
    scale_add = 0.0f;
    scale_max = 1.0f;
    draw_flags = SHOT_DRAW_MODEL | SHOT_DRAW_TRAIL;
    visual_code = 0;
}

void CLaserGun::SetVisualCode(s32 code) {
    visual_code = (s16) code;
    if (code == 0) {
        scale = 0.1f;
        scale_add = 0.1f;
        scale_max = 0.5f;
        color[0] = 64.0f;
        color[1] = 128.0f;
        color[2] = 64.0f;
    }
    if (code == 1) {
        scale = 0.2f;
        scale_add = 0.4f;
        scale_max = 0.8f;
        speed = 30.0f;
        color[0] = 64.0f;
        color[1] = 64.0f;
        color[2] = 128.0f;
    }
    if (code == 2) {
        scale = 0.2f;
        scale_add = 0.4f;
        scale_max = 1.4f;
        speed = 15.0f;
        speed_add = 5.0f;
        speed_max = 40.0f;
        color[0] = 128.0f;
        color[1] = 32.0f;
        color[2] = 128.0f;
    }
    if (code == 3) {
        scale = 0.2f;
        scale_add = 0.2f;
        scale_max = 0.6f;
        speed = 0.0f;
        speed_add = 2.0f;
        speed_max = 35.0f;
        homing_delay = 0;
        homing_time = 99999;
        life = 75;
        color[0] = 0.0f;
        color[1] = 128.0f;
        color[2] = 128.0f;
    }
    if (code == 4) {
        scale = 0.4f;
        scale_add = 0.4f;
        scale_max = 1.8f;
        speed = 10.0f;
        speed_add = 5.0f;
        speed_max = 30.0f;
        homing_delay = 0;
        homing_time = 5;
        life = 75;
        color[0] = 128.0f;
        color[1] = 64.0f;
        color[2] = 0.0f;
    }
}

void CLaserGun::Step() {
    CLaserGun *gun = this;
    float gravity = 0.1f;
    float distance = 30.0f;
    float spread = 50.0f;
    float power = 0.0f;
    sceVu0FVECTOR displacement;
    sceVu0FVECTOR previous_pos;
    sceVu0FVECTOR target_direction;
    CCPoly polygons[128];
    mgVu0FBOX bounds;
    sceVu0FVECTOR hit_pos;
    sceVu0FVECTOR map_effect_pos;
    sceVu0FVECTOR map_effect_color;
    sceVu0FVECTOR enemy_effect_pos;
    sceVu0FVECTOR enemy_effect_color;
    if (state != SHOT_STATE_FREE) {
        if (state == SHOT_STATE_FIRED) {
            state = SHOT_STATE_FLYING;
        }
        if (state == SHOT_STATE_FLYING) {
            CColPrim *collision = ColPrimMan.GetID2Prim(col_prim_id);
            if (homing_delay > 0) {
                homing_delay--;
            }
            if (homing_time > 0) {
                homing_time--;
            }
            life--;
            if (homing_delay <= 0) {
                if (homing_time > 0) {
                    if (target_chara != -1) {
                        CCharacter2 *target = DngMainScene->GetCharacter(target_chara);
                        if (target != NULL) {
                            target->GetEntryObjectPos(0, 0, target_pos);
                        }
                    }
                    sceVu0SubVector(target_direction, target_pos, pos);
                    sceVu0Normalize(target_direction, target_direction);
                    mgVectorInterpolate(dir, dir, target_direction, 0.05235988f, 0);
                }
            }
            sceVu0CopyVector(previous_pos, pos);
            sceVu0ScaleVector(displacement, dir, gun->speed);
            sceVu0AddVector(pos, pos, displacement);
            if (collision != NULL) {
                collision->SetCoord(pos, 5.0f);
            }
            bounds.max[3] = 1.0f;
            bounds.min[3] = 1.0f;
            bounds.max[0] = 20.0f + (pos[0] + gun->speed);
            bounds.min[0] = (pos[0] - gun->speed) - 20.0f;
            bounds.max[1] = 20.0f + (pos[1] + gun->speed);
            bounds.min[1] = (pos[1] - gun->speed) - 20.0f;
            bounds.max[2] = 20.0f + (pos[2] + gun->speed);
            bounds.min[2] = (pos[2] - gun->speed) - 20.0f;
            int polygon_count = DngMainMap->GetColPoly(polygons, bounds, 128);
            if (CheckHit(polygons, polygon_count, pos, previous_pos, hit_pos, 1, 4) >= 0) {
                state = SHOT_STATE_BURST;
                draw_flags &= ~SHOT_DRAW_MODEL;
                mgCCamera *camera = DngMainScene->GetCamera(DngMainScene->active_camera);
                if (camera != NULL) {
                    camera->GetPos(map_effect_pos);
                    sceVu0SubVector(map_effect_pos, map_effect_pos, pos);
                    sceVu0Normalize(map_effect_pos, map_effect_pos);
                    sceVu0ScaleVector(map_effect_pos, map_effect_pos, 20.0f);
                    sceVu0AddVector(map_effect_pos, pos, map_effect_pos);
                }
                sceVu0CopyVector(map_effect_color, color);
                map_effect_color[3] *= 1.2f;
                map_effect_color[3] *= 1.2f;
                map_effect_color[3] *= 1.2f;
                map_effect_color[3] = 240.0f;
                FxScriptMan->CreateEffSpt("\x82\x78\x83\x72\x81\x5B\x83\x80\x82\x67", 0, -1);
                FxScriptMan->SetScriptVect1(map_effect_pos, -1, -1);
                FxScriptMan->SetScriptVect2(color, -1, -1);
                FxScriptMan->SetValue(0, 1.8f, -1, -1);
                sndSePlay(DngMainScene->se_battle_id, 0x20, 0);
            }
            if (collision != NULL && collision->hit_num > 0) {
                state = SHOT_STATE_BURST;
                draw_flags &= ~SHOT_DRAW_MODEL;
                mgCCamera *camera = DngMainScene->GetCamera(DngMainScene->active_camera);
                if (camera != NULL) {
                    camera->GetPos(enemy_effect_pos);
                    sceVu0SubVector(enemy_effect_pos, enemy_effect_pos, pos);
                    sceVu0Normalize(enemy_effect_pos, enemy_effect_pos);
                    sceVu0ScaleVector(enemy_effect_pos, enemy_effect_pos, 20.0f);
                    sceVu0AddVector(enemy_effect_pos, pos, enemy_effect_pos);
                }
                sceVu0CopyVector(enemy_effect_color, color);
                enemy_effect_color[3] *= 1.4f;
                enemy_effect_color[3] *= 1.4f;
                enemy_effect_color[3] *= 1.4f;
                enemy_effect_color[3] = 240.0f;
                FxScriptMan->CreateEffSpt("\x82\x78\x83\x72\x81\x5B\x83\x80\x82\x67", 0, -1);
                FxScriptMan->SetScriptVect1(enemy_effect_pos, -1, -1);
                FxScriptMan->SetScriptVect2(color, -1, -1);
                FxScriptMan->SetValue(0, 1.8f, -1, -1);
                sndSePlay(DngMainScene->se_battle_id, 0x20, 0);
            }
            gun->speed += gun->speed_add;
            if (gun->speed > gun->speed_max) {
                gun->speed = gun->speed_max;
            }
            gun->scale += gun->scale_add;
            if (gun->scale > gun->scale_max) {
                gun->scale = gun->scale_max;
            }
            if (life <= 0) {
                state = SHOT_STATE_BURST;
                draw_flags &= ~SHOT_DRAW_MODEL;
            }
            if (state == SHOT_STATE_BURST) {
                if (life > 0) {
                    sceVu0FVECTOR hit_direction = {0.0f, 1.0f, 0.0f, 1.0f};
                    CHitEffectImage *hit_effect;
                    if (BattleFX.hit == NULL) {
                        hit_effect = NULL;
                    } else {
                        hit_effect = &BattleFX.hit[BattleFX.hit_next];
                        BattleFX.hit_next++;
                        if (BattleFX.hit_next >= BattleFX.hit_num) {
                            BattleFX.hit_next = 0;
                        }
                    }
                    if (hit_effect != NULL) {
                        hit_effect->SethitEffect(pos, hit_direction, spread, distance, power,
                                                gravity, 30, 32);
                        hit_effect->kind = HIT_EFFECT_SPARK_SHORT;
                    }
                }
                if (collision != NULL) {
                    collision->Delete(-1);
                }
            }
            trail_timer += 1;
            if (trail_timer >= 3) {
                sceVu0CopyVector(trail[trail_index], pos);
                trail_index += 1;
                if (trail_index >= 8) {
                    trail_index = 0;
                }
                trail_timer = 0;
                trail_len += 5;
            }
        }
        if (state == SHOT_STATE_BURST) {
            trail_len -= 1;
            if (trail_len < 3) {
                state = SHOT_STATE_FREE;
            }
        }
    }
}

void CLaserGun::Draw() {
    float size;
    int i;
    float alpha;
    float glow;
    int point_count;
    float interpolation;
    if (state != SHOT_STATE_FREE) {
        CPreSprite prim;
        sceVu0FVECTOR points[256];
        sceVu0IVECTOR top_left;
        sceVu0IVECTOR bottom_right;
        sceVu0FVECTOR previous_point;
        sceVu0FVECTOR interpolated_point;
        sceVu0FVECTOR segment;

        mgTexManager.ReloadTexture(tex_block, (sceVif1Packet *)NULL);
        if (draw_flags & SHOT_DRAW_TRAIL) {
            point_count = CreatSmoothPass(points, trail, 8, 6, trail_index, 8);
            if (point_count < trail_len) {
                trail_len = point_count;
            }
            prim.Initialize(NULL, NULL);
            prim.Preset2D();
            prim.AlphaBlendEnable(1);
            prim.AlphaTestEnable(1);
            prim.AlphaTest(1, 0);
            prim.DepthTestEnable(1);
            prim.ZMask(MG_Z_MASK_MASKED);
            prim.Bilinear(1);
            prim.TextureMapEnable(1);
            prim.Coord(1);
            prim.Begin(MG_PRIM_SPRITE);
            prim.Texture(trail_texture);
            prim.AlphaTestEnable(1);
            prim.SetAlphaBlend(MG_ALPHA_BLEND_ADD);
            prim.Color(0x80, 0x80, 0x80, 0x80);
            alpha = 1.0f;
            size = 6.0f * scale;
            for (i = point_count - 1; i >= point_count - trail_len + 1; i--) {
                points[i][3] = 1.0f;
                if (mgTransWorldPrim3DSprite(top_left, bottom_right, points[i], size, size, 0) != 0) {
                    int b;
                    int g;
                    int r;
                    r = (int)(96.0f + color[0]);
                    g = (int)(96.0f + color[1]);
                    b = (int)(96.0f + color[2]);
                    prim.Color(r, g, b, (int)(128.0f * alpha));
                    prim.TextureCrd(0, 0);
                    prim.Vertex4(top_left);
                    prim.TextureCrd(0x3F, 0x3F);
                    prim.Vertex4(bottom_right);
                }
                if (i != point_count - 1) {
                    interpolation = 0.1f;
                    int b;
                    int g;
                    int r;
                    int subdivision;
                    sceVu0SubVector(segment, points[i], previous_point);
                    for (subdivision = 0; subdivision < 9; subdivision++) {
                        sceVu0ScaleVector(interpolated_point, segment, interpolation);
                        sceVu0AddVector(interpolated_point, interpolated_point, previous_point);
                        if (mgTransWorldPrim3DSprite(top_left, bottom_right, interpolated_point, size, size, 0) !=
                            0) {
                            r = (int)(64.0f + color[0]);
                            g = (int)(64.0f + color[1]);
                            b = (int)(64.0f + color[2]);
                            prim.Color(r, g, b, (int)(128.0f * alpha));
                            prim.TextureCrd(0, 0);
                            prim.Vertex4(top_left);
                            prim.TextureCrd(0x3F, 0x3F);
                            prim.Vertex4(bottom_right);
                        }
                        interpolation += 0.1f;
                    }
                }
                sceVu0CopyVector(previous_point, points[i]);
                glow = 4.0f * size;
                if (mgTransWorldPrim3DSprite(top_left, bottom_right, points[i], glow, glow, 0) != 0) {
                    int b;
                    int g;
                    int r;
                    r = (int)(color[0]);
                    g = (int)(color[1]);
                    b = (int)(color[2]);
                    prim.Color(r, g, b, (int)(32.0f * alpha));
                    prim.TextureCrd(0, 0);
                    prim.Vertex4(top_left);
                    prim.TextureCrd(0x3F, 0x3F);
                    prim.Vertex4(bottom_right);
                }
                alpha -= 1.0f / (float)trail_len;
            }
            prim.End();
        }
        if (draw_flags & SHOT_DRAW_MODEL) {
            mgCFrameAttr attr;
            sceVu0FMATRIX matrix;
            attr.no_light = 1;
            attr.color[0] = color[0];
            attr.color[1] = color[1];
            attr.color[2] = color[2];
            attr.color[3] = 128.0f;
            model->SetAttrParam(attr, 1, MG_FRAME_ATTR_COLOR);
            model->SetScale(scale, scale, scale);
            model->SetPosition(pos);
            mgLookAtMatrixZ(matrix, dir);
            model->SetTransMatrix(matrix);
            mgDrawDirect(model);
        }
    }
}

void CLaserGun::Initialize(void) {
    target_chara = -1;
    trail_len = 0;
    trail_index = 0;
    state = SHOT_STATE_FREE;
    col_prim_id = -1;
    draw_flags = 0;
}

CLaserGun *CLaserGunMan::Get() {
    int i;

    for (i = 0; i < 16; i++) {
        if (laser[i].state == SHOT_STATE_FREE) {
            return &laser[i];
        }
    }
    return NULL;
}

void CLaserGunMan::Draw() {
    mgCTextureManager *textures;
    int                i;

    textures = &mgTexManager;
    for (i = 0; i < 16; i++) {
        textures->ReloadTexture(laser[i].tex_block, (sceVif1Packet *)NULL);
        laser[i].Draw();
    }
}

void CLaserGunMan::Step() {
    int i;

    for (i = 0; i < 16; i++) {
        laser[i].Step();
    }
}

void CLaserGunMan::Clear() {
    int i;

    for (i = 0; i < 16; i++) {
        laser[i].Initialize();
    }
}

void CLaserGunMan::Initialize(mgCFrame *model, int tex_block, mgCTexture *trail_texture) {
    int       i;
    CLaserGun *shot;

    for (i = 0; i < 16; i++) {
        laser[i].Initialize();
        laser[i].model = model;
        laser[i].tex_block = tex_block;
        laser[i].trail_texture = trail_texture;
    }
}

void CPullItem::Draw(mgCTexture *texture) {
    if (state == PULL_ITEM_STATE_FREE) {
        return;
    }

    CPreSprite    prim;
    sceVu0IVECTOR top_left;
    sceVu0IVECTOR bottom_right;
    sceVu0FVECTOR sprite_pos;
    float         glow_width;
    float         glow_height;
    int           texture_u;

    prim.Initialize(NULL, NULL);
    if (glow != 0) {
        prim.AlphaBlend(MG_ALPHA_BLEND_ADD);
    } else {
        prim.AlphaBlend(MG_ALPHA_BLEND_NORMAL);
    }
    prim.AlphaBlendEnable(1);
    prim.AlphaTestEnable(1);
    prim.AlphaTest(1, 0);
    prim.DepthTestEnable(1);
    prim.ZMask(MG_Z_MASK_MASKED);
    prim.Bilinear(1);
    prim.TextureMapEnable(1);
    prim.Coord(1);
    prim.Begin(MG_PRIM_SPRITE);
    prim.Texture(texture);
    prim.AlphaTestEnable(1);
    prim.Color(128, 128, 128, (int)alpha);
    pos[3] = 1.0f;
    texture_u = tex_u + (anim_frame / 4) * 16;
    sceVu0CopyVector(sprite_pos, pos);
    sprite_pos[1] += height / 2.0f;
    sprite_pos[1] += bob_height * sinf(angle);

    static float anim = 0.0f;

    if (anim > 3.1415927f) {
        anim = 0.0f;
    } else {
        anim += 0.20943952f;
    }
    glow_width = width + width * sinf(anim);
    glow_height = height + height * sinf(anim);
    sceVu0CopyVector(draw_pos, sprite_pos);
    if (glow != 0 && mgTransWorldPrim3DSprite(top_left, bottom_right, sprite_pos, glow_width, glow_height, 0) != 0) {
        prim.TextureCrd(97, 1);
        prim.Vertex4(top_left);
        prim.TextureCrd(127, 31);
        prim.Vertex4(bottom_right);
    }
    if (mgTransWorldPrim3DSprite(top_left, bottom_right, sprite_pos, width, height, 0) != 0) {
        prim.SetAlphaBlend(MG_ALPHA_BLEND_NORMAL);
        prim.TextureCrd(texture_u, tex_v);
        prim.Vertex4(top_left);
        prim.TextureCrd(texture_u + tex_w, tex_v + tex_h);
        prim.Vertex4(bottom_right);
    }
    prim.End();
}

#ifdef NONMATCHING
void CPullItem::Step() {
    CCharacter2  *player;
    sceVu0FVECTOR player_pos;
    sceVu0FVECTOR collect_pos;
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;
    sceVu0FVECTOR hit_pos;
    mgVu0FBOX     bounds;
    CCPoly       *polys;
    int           poly_count;
    float         ground_dist;
    sceVu0FVECTOR money_direction;
    char          badge_message[256];
    sceVu0FVECTOR badge_spark;
    char         *badge_name;
    sceVu0FVECTOR item_direction;
    sceVu0FVECTOR item_spark;
    char          item_message[256];
    int           item_count;
    sceVu0FVECTOR key_direction;
    char          key_message[256];
    sceVu0FVECTOR stolen_direction;
    char          stolen_message[256];
    sceVu0FVECTOR exp_direction;
    float         distance;

    if (state == PULL_ITEM_STATE_FREE) {
        return;
    }
    player = DngMainScene->GetCharacter(0);
    if (player == NULL) {
        return;
    }
    player->GetPosition(player_pos);
    player->GetPosition(collect_pos);
    collect_pos[1] += player->body_height;
    if (type == PULL_ITEM_MONEY || type == PULL_ITEM_WEAPON_EXP) {
        anim_frame++;
        if (anim_frame >= 16) {
            anim_frame = 0;
        }
    }
    if (can_get != 0 && get_delay > 0) {
        get_delay--;
    }
    if (state == PULL_ITEM_STATE_FALL) {
        bounds.max[0] = 60.0f + pos[0];
        bounds.min[0] = pos[0] - 60.0f;
        bounds.max[1] = 60.0f + pos[1];
        bounds.min[1] = pos[1] - 60.0f;
        bounds.max[2] = 60.0f + pos[2];
        bounds.min[2] = pos[2] - 60.0f;
        bounds.max[3] = 1.0f;
        bounds.min[3] = 1.0f;
        BuffWorkData__2.stack_used = 0;
        BuffWorkData__2.lock = 0;
        polys = (CCPoly *)BuffWorkData__2.stAlloc64(641);
        poly_count = DngMainScene->GetColPoly(polys, bounds, 128);
        sceVu0CopyVector(from, pos);
        sceVu0AddVector(to, pos, velocity);
        if (CheckHit(polys, poly_count, from, to, hit_pos, 1, 0x4) > 0) {
            pos[0] = hit_pos[0];
            pos[2] = hit_pos[2];
            velocity[0] *= -0.6f;
            velocity[2] *= -0.6f;
        }
        sceVu0AddVector(pos, pos, velocity);
        if (velocity[1] > -3.0f) {
            velocity[1] -= 0.3f;
        }
        sceVu0CopyVector(from, pos);
        sceVu0CopyVector(to, pos);
        from[1] += 5.0f;
        to[1] -= 20.0f;
        if (CheckHit(polys, poly_count, from, to, hit_pos, 1, 0x4) >= 0) {
            ground_dist = pos[1] - hit_pos[1];
            if (ground_dist <= 1.0f) {
                can_get = 1;
                velocity[1] *= -0.6f;
                if (ground_dist <= 0.0f) {
                    pos[1] = hit_pos[1];
                }
                if (velocity[1] < 1.0f) {
                    state = PULL_ITEM_STATE_LAND;
                }
            }
        }
        fall_time--;
        if (fall_time <= 0) {
            state = PULL_ITEM_STATE_FADE;
            wait_time = 30;
        }
    }
    if (state == PULL_ITEM_STATE_LAND) {
        if (wait_time > 0) {
            wait_time--;
        }
        if (wait_time == 0) {
            wait_time = 30;
            state = PULL_ITEM_STATE_FADE;
        }
    }
    if (state == PULL_ITEM_STATE_FADE) {
        wait_time--;
        alpha -= 4.266667f;
        if (wait_time <= 0) {
            state = PULL_ITEM_STATE_FREE;
            if (wire_index >= 0) {
                afterWire[wire_index].SetMode(0);
                wire_index = -1;
            }
        }
    }
    if (state == PULL_ITEM_STATE_GOT) {
        sceVu0CopyVector(pos, collect_pos);
        get_delay--;
        if (get_delay <= 0) {
            state = PULL_ITEM_STATE_FREE;
        }
        if (angle < 2.3561945f) {
            angle += 0.1308997f;
        }
    }
    if (state == PULL_ITEM_STATE_FLOAT) {
        if (get_delay > 0) {
            get_delay--;
            angle += 0.20943952f;
            if (angle >= 3.1415927f) {
                angle -= 6.2831855f;
            }
        } else {
            can_get = 1;
            angle = 0.0f;
        }
    }
    if (state == PULL_ITEM_STATE_COLLECT) {
        if (type == PULL_ITEM_MONEY || type == PULL_ITEM_MONEY_LARGE) {
            angle += 0.15707964f;
            sceVu0SubVector(money_direction, player_pos, pos);
            sceVu0Normalize(money_direction, money_direction);
            sceVu0ScaleVectorXYZ(money_direction, money_direction, 2.5f);
            sceVu0AddVector(pos, pos, money_direction);
            distance = mgDistVector(player_pos, pos);
            if (angle >= 3.1415927f || distance <= 5.0f) {
                DngUserData->AddMoney(item_no);
                state = PULL_ITEM_STATE_FREE;
                sndSePlay(DngMainScene->se_battle_id, 3, 0);
            }
        }
        if (type == PULL_ITEM_BADGE) {
            badge_name = mons_attr_list[LanguageCode][item_no];
            if (DngUserData->monster_box.IsChange(item_no) != 0) {
                sprintf(badge_message, dung_progtxt_badge_already[LanguageCode], badge_name);
                MsgTaskMan.Print(badge_message, 90, 8, 0);
                state = PULL_ITEM_STATE_FREE;
            } else {
                angle += 0.15707964f;
                if (angle >= 3.1415927f) {
                    sprintf(badge_message, dung_progtxt_badge_get[LanguageCode], badge_name);
                    MsgTaskMan.Print(badge_message, 60, 8, 0);
                    DngUserData->monster_box.EnableChange(item_no);
                    state = PULL_ITEM_STATE_FREE;
                }
                sceVu0CopyVector(badge_spark, draw_pos);
                badge_spark[0] += fRand(10.0f) - 5.0f;
                badge_spark[1] -= fRand(3.0f);
                badge_spark[2] += fRand(10.0f) - 5.0f;
                MiniEffPrimMan.CreatPrim(badge_spark, 0);
            }
        }
        if (type == PULL_ITEM_ITEM || type == PULL_ITEM_ITEM2) {
            if (mgDistVector(collect_pos, pos) > 5.0f) {
                angle += 0.15707964f;
                sceVu0SubVector(item_direction, player_pos, pos);
                sceVu0Normalize(item_direction, item_direction);
                sceVu0ScaleVectorXYZ(item_direction, item_direction, 2.5f);
                sceVu0AddVector(pos, pos, item_direction);
            }
            sceVu0CopyVector(item_spark, draw_pos);
            item_spark[0] += fRand(10.0f) - 5.0f;
            item_spark[1] -= fRand(3.0f);
            item_spark[2] += fRand(10.0f) - 5.0f;
            MiniEffPrimMan.CreatPrim(item_spark, 0);
            if (angle >= 3.1415927f) {
                angle = 0.0f;
                num = 1;
                if (CheckGetItemLimmitOver(item_no, num) < num) {
                    item_count = num;
                    if (LanguageCode == 0) {
                        sprintf(item_message, dung_progtxt_getitem_overnum[0][1], GetItemMessage(item_no), item_count);
                    } else if (item_count < 2) {
                        sprintf(item_message, dung_progtxt_getitem_overnum[LanguageCode][0], GetItemMessage(item_no));
                    } else {
                        sprintf(item_message, dung_progtxt_getitem_overnum[LanguageCode][1], num, GetItemMessage(item_no));
                    }
                    MsgTaskMan.Print(item_message, 90, 8, 0);
                    state = PULL_ITEM_STATE_LAND;
                    can_get = 1;
                    get_delay = 60;
                    wait_time = 300;
                } else {
                    item_count = num;
                    if (LanguageCode == 0) {
                        sprintf(item_message, dung_progtxt_getitem[0][1], GetItemMessage(item_no), item_count);
                    } else if (item_count < 2) {
                        sprintf(item_message, dung_progtxt_getitem[LanguageCode][0], GetItemMessage(item_no));
                    } else {
                        sprintf(item_message, dung_progtxt_getitem[LanguageCode][1], num, GetItemMessage(item_no));
                    }
                    MsgTaskMan.Print(item_message, 45, 8, 0);
                    DngUserData->GetItem(item_no, 1);
                    state = PULL_ITEM_STATE_FREE;
                }
            }
        }
        if (type == PULL_ITEM_GATE_KEY) {
            if (mgDistVector(collect_pos, pos) > 5.0f + pull_speed) {
                pull_speed += pull_accel;
                pull_speed += 0.2f;
                sceVu0SubVector(key_direction, collect_pos, pos);
                sceVu0Normalize(key_direction, key_direction);
                sceVu0ScaleVectorXYZ(key_direction, key_direction, pull_speed);
                key_direction[1] += 2.5f;
                sceVu0AddVector(pos, pos, key_direction);
            } else {
                sprintf(key_message, dung_progtxt_gkey_get[LanguageCode], GetItemMessage(item_no));
                MsgTaskMan.Print(key_message, 90, 8, 0);
                DngUserData->GetItem(item_no, 1);
                state = PULL_ITEM_STATE_GOT;
                get_delay = 60;
                bob_height = 30.0f;
            }
        }
        if (type == PULL_ITEM_STOLEN) {
            if (mgDistVector(collect_pos, pos) > 5.0f + pull_speed) {
                pull_speed += pull_accel;
                pull_speed += 0.2f;
                sceVu0SubVector(stolen_direction, collect_pos, pos);
                sceVu0Normalize(stolen_direction, stolen_direction);
                sceVu0ScaleVectorXYZ(stolen_direction, stolen_direction, pull_speed);
                stolen_direction[1] += 2.5f;
                sceVu0AddVector(pos, pos, stolen_direction);
            } else {
                sprintf(stolen_message, dung_progtxt_steal[LanguageCode], GetItemMessage(item_no));
                MsgTaskMan.Print(stolen_message, 90, 8, 0);
                DngUserData->GetItem(item_no, 1);
                state = PULL_ITEM_STATE_GOT;
                get_delay = 60;
                bob_height = 30.0f;
            }
        }
        if (type == PULL_ITEM_WEAPON_EXP) {
            if (mgDistVector(collect_pos, pos) > 5.0f + pull_speed) {
                pull_speed += pull_accel;
                pull_speed += 0.2f;
                sceVu0SubVector(exp_direction, collect_pos, pos);
                sceVu0Normalize(exp_direction, exp_direction);
                sceVu0ScaleVectorXYZ(exp_direction, exp_direction, pull_speed);
                exp_direction[1] += 2.5f;
                sceVu0AddVector(pos, pos, exp_direction);
            } else {
                AddExpWeaponParam(exp, exp_param, item_no);
                sndSePlay(DngMainScene->se_battle_id, 4, 0);
                state = PULL_ITEM_STATE_FREE;
                if (wire_index >= 0) {
                    afterWire[wire_index].SetMode(0);
                    wire_index = -1;
                }
            }
        }
    }
    if (wire_index >= 0) {
        afterWire[wire_index].SetPos(pos);
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_object", Step__9CPullItemFv);
#endif

#ifdef NONMATCHING
void CPullItem::IsGet(float *player_pos) {
    if (state != PULL_ITEM_STATE_FREE && can_get != 0) {
        if (get_delay > 0) {
            return;
        }
        if (type == PULL_ITEM_GATE_KEY || type == PULL_ITEM_STOLEN) {
            can_get = 0;
            state = PULL_ITEM_STATE_COLLECT;
            return;
        }
        if (mgDistVector(player_pos, pos) < 20.0f * get_range) {
            state = PULL_ITEM_STATE_COLLECT;
            can_get = 0;
            if (type == PULL_ITEM_ITEM || type == PULL_ITEM_BADGE) {
                sndSePlay(SystemSND_ID, 18, 0);
            }
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_object", IsGet__9CPullItemFPf);
#endif

void CPullItem::SetItem(float *position, float *velo, int item_type) {
    int index;

    sceVu0CopyVector(pos, position);
    sceVu0CopyVector(velocity, velo);
    type = item_type;
    angle = 0.0f;
    can_get = 0;
    fall_time = 300;
    exp_param = -1;
    item_no = -1;
    alpha = 128.0f;
    glow = 0;
    bob_height = 15.0f;
    wire_index = -1;
    switch (item_type) {
        case PULL_ITEM_GATE_KEY:
            state = PULL_ITEM_STATE_FLOAT;
            tex_u = 0;
            tex_v = 0;
            tex_h = 32;
            tex_w = 32;
            width = 7.0f;
            height = 8.0f;
            get_range = 0.8f;
            anim_frame = 0;
            get_delay = 30;
            pull_speed = 1.8f;
            pull_accel = 0.1f;
            bob_height = 4.0f;
            glow = 1;
            break;
        case PULL_ITEM_STOLEN:
            state = PULL_ITEM_STATE_FLOAT;
            tex_u = 97;
            tex_v = 33;
            tex_h = 30;
            tex_w = 30;
            width = 7.0f;
            height = 8.0f;
            get_range = 0.8f;
            anim_frame = 0;
            get_delay = 30;
            pull_speed = 1.8f;
            pull_accel = 0.1f;
            bob_height = 4.0f;
            glow = 1;
            break;
        case PULL_ITEM_MONEY_LARGE:
            state = PULL_ITEM_STATE_FALL;
            tex_u = 32;
            tex_v = 0;
            tex_h = 32;
            tex_w = 32;
            width = 7.0f;
            height = 8.0f;
            get_range = 4.5f;
            anim_frame = 0;
            get_delay = 40;
            wait_time = 240;
            break;
        case PULL_ITEM_MONEY:
            state = PULL_ITEM_STATE_FALL;
            tex_u = 0;
            tex_v = 32;
            tex_h = 16;
            tex_w = 16;
            width = 3.0f;
            height = 4.0f;
            get_range = 4.5f;
            anim_frame = iRand(16);
            get_delay = 40;
            wait_time = 180;
            break;
        case PULL_ITEM_WEAPON_EXP:
            state = PULL_ITEM_STATE_FALL;
            tex_u = 1;
            tex_v = 49;
            tex_h = 14;
            tex_w = 14;
            width = 3.0f;
            height = 3.0f;
            get_range = 4.5f;
            get_delay = 30;
            wait_time = 180;
            pull_speed = 1.8f;
            pull_accel = 0.1f;
            glow = 1;
            for (index = 0; index < 16; index++) {
                if (afterWire[index].mode == 0) {
                    afterWire[index].SetMode(1);
                    wire_index = index;
                    break;
                }
            }
            break;
        case PULL_ITEM_ITEM:
            state = PULL_ITEM_STATE_FALL;
            tex_u = 64;
            tex_v = 0;
            tex_h = 31;
            tex_w = 31;
            width = 7.0f;
            height = 8.0f;
            get_range = 4.5f;
            anim_frame = 0;
            get_delay = 10;
            wait_time = 300;
            bob_height = 25.0f;
            break;
        case PULL_ITEM_ITEM2:
            state = PULL_ITEM_STATE_FALL;
            tex_u = 64;
            tex_v = 32;
            tex_h = 31;
            tex_w = 31;
            width = 7.0f;
            height = 7.0f;
            get_range = 4.5f;
            anim_frame = 0;
            get_delay = 10;
            wait_time = 300;
            bob_height = 25.0f;
            break;
        case PULL_ITEM_BADGE:
            state = PULL_ITEM_STATE_FALL;
            tex_u = 64;
            tex_v = 0;
            tex_h = 31;
            tex_w = 31;
            width = 7.0f;
            height = 8.0f;
            get_range = 1.5f;
            anim_frame = 0;
            get_delay = 10;
            wait_time = 300;
            bob_height = 25.0f;
            break;
    }
}

void CPullItem::Clear(void) {
    wire_index = -1;
    state = PULL_ITEM_STATE_FREE;
}

void CPullItem::Initialize(void) {
    state = PULL_ITEM_STATE_FREE;
    wait_time = 0;
    tex_u = 0;
    tex_v = 0;
    tex_w = 32;
    tex_h = 32;
    can_get = 0;
    anim_frame = 0;
}

CPullItem *CPullItemManager::GetList(int start) {
    CPullItem *item;
    int        i;

    if (list == NULL || num <= 0) {
        return NULL;
    }
    item = &list[start];
    for (i = start; i < num; item++, i++) {
        if (item->state == PULL_ITEM_STATE_FREE) {
            return item;
        }
    }
    return NULL;
}

void CPullItemManager::Clear() {
    int i;

    if (list != NULL) {
        for (i = 0; i < num; i++) {
            list[i].Clear();
        }
    }
}

void CRoboVoiceSystem::SetStatus(s32 voice, s32 value) {
    status = 1;
    voice_no = voice;
    unk_10 = value;
}

void CRoboVoiceSystem::StartVoiceSystem() {
    status = ROBO_VOICE_WAIT;
    voice_no = -1;
    wait_time = iRand(240) + 60;
    stream_open = 0;
}

void CRoboVoiceSystem::StopVoice(int pause) {
    if (stream_open != 0) {
        while (CSnd.StreamOpenState() != 0) {
        }

        CSnd.StreamClose(1);
    }

    stream_open = 0;
    status = ROBO_VOICE_OFF;
    pause_time = pause;
}

#ifdef NONMATCHING
void CRoboVoiceSystem::Step() {
    CBattleCharaInfo *battle_info;
    int               max_hp;
    int               now_hp;
    int               now_whp;
    float             hp_ratio;
    int               monster_count;
    int               healthy_voices[4] = {30, 40, 60, 160};
    int               injured_voices[3] = {80, 90, 190};
    int               low_hp_voices[5] = {100, 110, 120, 170, 220};
    int               long_play_voices[3] = {70, 140, 150};
    int               nearby_voices[2] = {50, 180};
    int               crowded_voices[2] = {130, 200};
    int               critical_voices[4] = {120, 120, 220, 170};
    char              voice_file[64];

    if (status == ROBO_VOICE_OFF) {
        return;
    }

    if (pause_time > 0) {
        pause_time--;
        return;
    }

    battle_info = GetBattleCharaInfo();
    max_hp = battle_info->GetMaxHp_i();
    now_hp = battle_info->GetNowHp_i();
    battle_info->GetNowWhp(0, &now_whp);
    play_time++;
    hp_ratio = (float)now_hp / (float)max_hp;

    switch (status) {
        case ROBO_VOICE_WAIT:
            wait_time--;
            if (wait_time < 0) {
                wait_time = 0;
                if (voice_no == -1) {
                    if (hp_ratio > 0.8f) {
                        SetStatus(healthy_voices[iRand(4)], 0);
                    }

                    if (hp_ratio <= 0.8f && hp_ratio > 0.4f) {
                        SetStatus(injured_voices[iRand(3)], 0);
                    }

                    if (hp_ratio <= 0.4f) {
                        SetStatus(low_hp_voices[iRand(5)], 0);
                    }

                    if (iRand(100) % 4 == 0 && play_time >= 3600) {
                        SetStatus(long_play_voices[iRand(3)], 0);
                    }

                    if (iRand(100) % 2 != 0) {
                        monster_count = ActiveMonster->GetMonsterNum(340.0f);
                        if (monster_count > 0) {
                            if (monster_count >= 4) {
                                SetStatus(crowded_voices[iRand(2)], 0);
                            } else {
                                SetStatus(nearby_voices[iRand(2)], 0);
                            }
                        }
                    }

                    if (hp_ratio <= 0.2f) {
                        SetStatus(critical_voices[iRand(4)], 0);
                    }
                } else {
                    SetStatus(voice_no, 0);
                }
            }
            break;

        case ROBO_VOICE_OPEN:
            if (voice_no < 100) {
                sprintf(voice_file, "85200%d.wav", voice_no);
            } else {
                sprintf(voice_file, "8520%d.wav", voice_no);
            }

            CSnd.StreamOpenFast(1, voice_file);
            status = ROBO_VOICE_OPENING;
            stream_open = 1;
            break;

        case ROBO_VOICE_OPENING:
            if (CSnd.StreamOpenState() == 0) {
                CSnd.StreamStandBy(1);
                status = ROBO_VOICE_STANDBY;
            }
            break;

        case ROBO_VOICE_STANDBY:
            if (CSnd.StreamOpenState() == 0) {
                CSnd.StreamSetVol(1, 0x7FFF, 0x7FFF);
                CSnd.StreamPlay(1);
                status = ROBO_VOICE_PLAY;
            }
            break;

        case ROBO_VOICE_PLAY:
            if (CSnd.StreamGetState(1) == 0x8000) {
                CSnd.StreamClose(1);
                status = ROBO_VOICE_WAIT;
                stream_open = 0;
                wait_time = iRand(300) + 90;
                if (voice_no == 200) {
                    voice_no = 210;
                    wait_time = 45;
                } else {
                    voice_no = -1;
                }
            }
            break;
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_object", Step__16CRoboVoiceSystemFv);
#endif


// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_923__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1112__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1240__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", mons_attr_tbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", mons_attr_tbl2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", mons_attr_tbl3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", mons_attr_tbl4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", mons_attr_tbl5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", mons_attr_tbl6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", mons_attr_list__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", dung_progtxt_badge_already__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", dung_progtxt_badge_get__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", dung_progtxt_gkey_get__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", dung_progtxt_steal__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", dung_progtxt_getitem_overnum__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", dung_progtxt_getitem__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1800__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1801__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1802__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1803__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1806__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_961__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1291__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1428__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1429__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1430__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1431__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1432__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1433__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1434__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1435__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1436__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1437__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1438__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1439__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1440__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1441__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1442__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1443__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1444__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1445__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1446__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1447__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1448__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1449__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1450__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1451__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1452__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1453__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1454__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1455__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1456__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1457__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1458__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1459__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1460__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1461__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1462__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1463__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1464__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1465__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1466__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1467__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1468__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1469__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1470__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1471__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1472__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1473__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1474__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1475__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1476__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1477__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1478__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1479__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1480__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1481__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1482__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1483__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1484__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1485__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1486__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1487__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1488__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1489__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1490__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1491__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1492__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1493__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1494__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1495__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1496__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1497__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1498__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1499__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1500__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1501__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1502__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1503__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1504__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1505__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1506__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1507__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1508__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1509__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1510__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1511__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1512__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1513__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1514__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1515__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1516__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1517__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1518__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1519__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1520__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1521__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1522__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1523__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1524__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1525__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1526__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1527__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1528__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1529__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1530__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1531__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1736__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1853__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1854__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1804__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_object", at_1805__DATA);

// Small uninitialised data (.sbss)
