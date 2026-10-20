// N6/N7: the allocation points the imported room's exact effect and primitive
// ownership is fed from. Each is the retail body with the running effect
// script identified by its operand stack (instead of the file-static
// now_script of ps2/src/effscript.cpp, which is that same script) and one
// bookkeeping call added.
#include "common.h"
#include "mw_runtime.h"

#include <cstdio>

#include "colprim.hpp"
#include "dng_main.hpp"
#include "effscript.hpp"
#include "ixion_room.hpp"
#include "ixion_toan_combat.hpp"
#include "runscript.hpp"
#include "scene.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"

void SetEffectScriptFunc();

namespace {
int StackInt(RS_STACKDATA *slot) {
    if (slot->type == RS_FLOAT) return fptosi(slot->val.f);
    return slot->val.i;
}

char *StackString(RS_STACKDATA *slot) {
    return slot->val.s;
}

void StackSet(RS_STACKDATA *slot, int value) {
    if (slot->type == RS_PTR) slot->val.p->val.i = value;
}

_EFF_SCRIPT *Running(RS_STACKDATA *stack, const char *command) {
    _EFF_SCRIPT *script = IxionRoomRunningEffect(EffScriptMan, stack);
    if (script == nullptr) {
        std::fprintf(stderr, "ixion room INVARIANT: %s called outside a running effect script\n", command);
    }
    return script;
}
} // namespace

// Every hand-out ends the previous holder's claim on the slot.
PORT_OVERRIDE CColPrim *CColPrimMan::GetPrim() {
    for (int i = 0; i < COLPRIM_MAX; ++i) {
        if (!prim[i].active) {
            IxionToanCombatPrimInvalidated(&prim[i]);
            prim[i].id = i;
            if (this == &ColPrimMan) IxionRoomNotePrimHandedOut(&prim[i]);
            return &prim[i];
        }
    }

    return NULL;
}

PORT_OVERRIDE int _COLPRIM_CREATE(RS_STACKDATA *stack, int argc) {
    _EFF_SCRIPT *now_script = Running(stack, "_COLPRIM_CREATE");
    if (now_script == nullptr) return 0;
    CColPrim *colprim;
    int       owner;
    char     *damage_name;

    if (now_script->colprim != NULL) {
        now_script->colprim->Delete(now_script->user_id);
    }

    colprim = ColPrimMan.GetPrim();
    now_script->colprim = colprim;

    if (colprim == NULL) {
        return 0;
    }

    owner = now_script->user_id;
    damage_name = StackString(stack++);

    if (argc >= 2) {
        owner = StackInt(stack);
    }

    now_script->colprim->SetDamage(damage_name, owner);
    IxionRoomNoteEffectPrim(colprim, now_script); // SetDamage initializes the new incarnation first.
    return 1;
}

// A child inherits its parent's recorded source, whatever user_id it is filed under.
PORT_OVERRIDE int _ES_CREATE(RS_STACKDATA *stack, int argc) {
    _EFF_SCRIPT *now_script = Running(stack, "_ES_CREATE");
    if (now_script == nullptr) return 0;
    int   handle = -1;
    char *name = StackString(stack++);

    switch (argc) {
        case 1:
            IxionRoomSetEffectParent(now_script);
            EffScriptMan->CreateEffSpt(name, now_script->user_id, 0);
            IxionRoomSetEffectParent(nullptr);
            break;
        case 2: {
            int user_id = now_script->user_id;

            if (user_id >= 0) {
                IxionRoomSetEffectParent(now_script);
                handle = EffScriptMan->CreateEffSpt(name, user_id, 1);
                IxionRoomSetEffectParent(nullptr);
            }

            StackSet(stack, handle);

            if (handle <= -1) {
                printf("ES_CREATE error [%s][%d][%d]", name, now_script->user_id, handle);
            }

            break;
        }
        default:
            return 0;
    }

    return 1;
}

// Retail body with adapter borrower resolution before lifetime revocation.
// Without an adapter origin, the hook leaves native state/deletion unchanged.
PORT_OVERRIDE void CEffectScriptMan::DeleteEffSpt(_EFF_SCRIPT *script) {
    if (script == 0 || work_memory == 0) {
        return;
    }

    IxionRoomBeforeEffectDelete(script);

    if (script->prev != 0) {
        script->prev->next = script->next;
    } else {
        head = script->next;

        if (head != 0) {
            head->prev = 0;
        }
    }

    if (script->next != 0) {
        script->next->prev = script->prev;
    } else {
        tail = script->prev;

        if (tail != 0) {
            tail->next = 0;
        }
    }

    if (now != 0 && script->work == now->work) {
        now = 0;
    }

    if (script->slot >= 0) {
        slot[script->user_id][script->slot] = 0;
    }

    if (script->colprim != 0) {
        script->colprim->Delete(script->user_id);
    }

    DeleteSprite(script->sprite);

    if (script->sub_chara_work != 0) {
        work_memory->Free(script->sub_chara_work);
    }

    if (script->chara_work != 0) {
        work_memory->Free(script->chara_work);
    }

    work_memory->Free(script->work);
}

// Retail body plus lifetime revocation; no native state changes from bookkeeping.
PORT_OVERRIDE void CEffectScriptMan::Initialize(mgCMemory *memory, int texb_start, int texb_num) {
    if (this == FxScriptMan || this == EffScriptMan) IxionRoomClearAttackClaims();
    int                i;
    int                j;
    mgCTextureManager *manager;
    int                bank;

    this->memory = memory;
    load_buffer = 0;
    work_memory = 0;
    level = 0;

    for (i = 0; i < EFF_SPT_BASE_MAX; i++) {
        base[i] = 0;
    }

    base_num = 0;

    for (i = 0; i < EFF_SPT_OWNER_MAX; i++) {
        for (j = 0; j < EFF_SPT_OWNER_SLOT_MAX; j++) {
            slot[i][j] = 0;
        }
    }

    now = 0;
    this->texb_start = texb_start;
    this->texb_num = texb_num;
    texb_used = 0;
    tail = 0;
    head = 0;
    now_scene = GetMainScene();
    SetEffectScriptFunc();
    level_texb_used[0] = 0;
    level_texb_used[1] = 0;
    level_texb_used[2] = 0;
    level_texb_used[3] = 0;
    manager = &mgTexManager;

    for (bank = this->texb_start; bank < this->texb_start + this->texb_num; bank++) {
        manager->DeleteBlock(bank);
    }
}

// Retail body plus lifetime revocation; no native state changes from bookkeeping.
PORT_OVERRIDE void CColPrim::Initialize() {
    IxionToanCombatPrimInvalidated(this);
    IxionRoomNotePrimHandedOut(this);
    active = false;
    owner = -1;
    hit_mask = 0;
    step_count = 0;
    life = -1;
    hit_num = 0;
    reversed = false;
    has_gift = false;
    unk_34 = 0;
    frame[1] = NULL;
    frame[0] = NULL;
    radius = 0.0f;
    unk_8c = -1;
}

// Retail body plus lifetime revocation; no native state changes from bookkeeping.
PORT_OVERRIDE void CColPrim::Delete(int id) {
    if (active && (id == -1 || owner == id)) IxionToanCombatPrimInvalidated(this);
    // Native owner check remains authoritative.
    if (active) {
        if (id == -1) {
            active = false;
        } else if (owner == id) {
            active = false;
        }
    }
    if (!active) IxionRoomNotePrimHandedOut(this);
}
