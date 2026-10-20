#include "common.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "menuchr.hpp"
#include "monster.hpp"
#include "userdata.hpp"

// Ixion opt-in (DC2_IXION_MONICA_FORM=<DC1 model>, for example e104a): Monica's Himarra form
// (monster 52, act script f216a) wears a converted DC1 body, ix_<model>.chr with ix_<model>.cfg,
// supplied by the DC2_IXION_EXTRA_DATA overlay. Everything else stays DC2's own: the badge menu,
// the transform, SetupMainUnit and the menu loader, the form's act script, its damage table and
// HUD. Only the model and config names change. Enemies never come here: monster spawns read
// BASE_MONSTER_TBL::model directly (monster.cpp), and the base row is not written.
namespace {

constexpr int kMonicaFormMonster = 52; // Himarra, badge 4's first form (monster_progress_tbl row 4)

const char *MonicaFormModel() {
    static bool        read = false;
    static char        model[16] = {};
    if (!read) {
        read = true;
        const char *value = std::getenv("DC2_IXION_MONICA_FORM");
        if (value != nullptr && *value != '\0') {
            const int written = std::snprintf(model, sizeof model, "ix_%s", value);
            if (written <= 0 || written >= static_cast<int>(sizeof model)) {
                std::fprintf(stderr, "ixion monica form: DC2_IXION_MONICA_FORM=%s is too long; ignored\n", value);
                model[0] = '\0';
            } else {
                std::fprintf(stderr, "ixion monica form: monster %d wears %s\n", kMonicaFormMonster, model);
            }
        }
    }
    return model[0] != '\0' ? model : nullptr;
}

void LogRedirect(int kind, const char *file_name) {
    static int logged[4] = {};
    if (kind >= 0 && kind < 4 && logged[kind] < 8) {
        ++logged[kind];
        std::fprintf(stderr, "ixion monica form: monster %d kind=%d -> %s\n", kMonicaFormMonster, kind, file_name);
    }
}

} // namespace

// Verbatim ps2/src/menuchr.cpp GetMonsterModelFile, plus the opt-in model/config redirect for the
// player's Himarra form. Script and sound stay the native form's (f216a, Himarra's bank).
PORT_OVERRIDE int GetMonsterModelFile(int monster_id, int kind, char *file_name) {
    char              suffix[0x20];
    BASE_MONSTER_TBL *monster;
    char             *base_name;
    int               number;
    MOS_HENGE_PARAM  *henge_param;

    if (file_name == NULL) {
        return 0;
    }

    monster = GetMonsterTable(monster_id);
    base_name = monster->model;

    if (monster == NULL) {
        return 0;
    }

    if ((int) strlen(base_name) <= 0) {
        return 0;
    }

    const char *ixion_model = monster_id == kMonicaFormMonster ? MonicaFormModel() : nullptr;

    if (ixion_model != nullptr && (kind == MONSTER_FILE_MODEL || kind == MONSTER_FILE_CONFIG)) {
        base_name = (char *) ixion_model;
    }

    strcpy(file_name, base_name);

    if (kind == MONSTER_FILE_MODEL) {
        strcat(file_name, ".chr");
    }

    if (kind == MONSTER_FILE_SOUND) {
        number = monster->sound_no;

        if (number < 0) {
            return 0;
        }

        strcpy(file_name, "snd2/mon/");

        if (number < 10) {
            sprintf(suffix, "EN_00%d.snd", number);
        } else if (number < 100) {
            sprintf(suffix, "EN_0%d.snd", number);
        } else {
            sprintf(suffix, "EN_%d.snd", number);
        }

        strcat(file_name, suffix);
    }

    if (kind == MONSTER_FILE_SCRIPT) {
        henge_param = GetMonsterHengeParam(monster_id);

        if (henge_param != NULL) {
            sprintf(file_name, "%s.stb", henge_param->script_name);
        }
    }

    if (kind == MONSTER_FILE_CONFIG) {
        strcpy(file_name, base_name);
        strcat(file_name, ".cfg");
    }

    if (ixion_model != nullptr && (kind == MONSTER_FILE_MODEL || kind == MONSTER_FILE_CONFIG)) {
        LogRedirect(kind, file_name);
    }

    return 1;
}
