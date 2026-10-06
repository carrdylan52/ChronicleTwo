#include "common.h"
#include "quest.hpp"
#include "mainloop.hpp"
#include "savedata.hpp"
#include "scriptinterpreter.hpp"
#include "mg_memory.hpp"
#include <cstring>

static CQuestManager *spi_questman; /**< Request list currently being read from a script. */
static mgCMemory *spi_queststack; /**< Heap used for the request list. */
static QUEST_INFO *spi_quest_info; /**< Request currently being filled. */

static int quest_NUM(SPI_STACK *arguments, int argument_count);
static int quest_NEW(SPI_STACK *arguments, int argument_count);
static int quest_COMMENT(SPI_STACK *arguments, int argument_count);
static int quest_END(SPI_STACK *arguments, int argument_count);

static SPI_TAG_PARAM quest_cmd_tag[] = {
    {"NUM", quest_NUM},
    {"NEW", quest_NEW},
    {"COMENT", quest_COMMENT},
    {"END", quest_END},
    {NULL, NULL},
};

// Code (.text)
/**
 * Returns the saved request progress, or null when no save data is active.
 */
static CQuestData *GetQuestData() {
    CSaveData *save_data = GetSaveData();
    return save_data != NULL ? &save_data->quest_data : NULL;
}

void CQuestManager::Initialize(void) {
    num = 0;
    info = NULL;
}

QUEST_INFO *CQuestManager::GetQuestInfo(int id) {
    for (int index = 0; index < num; ++index) {
        if (info[index].id == id) {
            return &info[index];
        }
    }
    return NULL;
}

/**
 * Allocates the request list specified by the script.
 */
static int quest_NUM(SPI_STACK *arguments, int argument_count) {
    int request_count;
    unsigned int allocation_size;
    int allocation_quads;

    request_count = spiGetStackInt(arguments);
    spi_questman->num = request_count;
    allocation_size = request_count * sizeof(QUEST_INFO);
    if (allocation_size & 0xF) {
        allocation_quads = (allocation_size >> 4) + 1;
    } else {
        allocation_quads = allocation_size >> 4;
    }
    spi_questman->info =
        new (spi_queststack->Alloc(allocation_quads + 2)) QUEST_INFO[request_count];
    spi_quest_info = spi_questman->info;
    return 1;
}

/**
 * Sets the identifier and name of the current request.
 */
static int quest_NEW(SPI_STACK *arguments, int argument_count) {
    int id = spiGetStackInt(arguments++);
    char *name = spiGetStackString(arguments);
    spi_quest_info->id = id;
    strcpy(spi_quest_info->name, name);
    return 1;
}

/**
 * Sets the comment or reaction of the current request.
 */
static int quest_COMMENT(SPI_STACK *arguments, int argument_count) {
    int comment_index = spiGetStackInt(arguments++);
    char *comment = spiGetStackString(arguments);
    if (comment_index == 0) {
        strcpy(spi_quest_info->comment, comment);
    }
    if (comment_index > 0) {
        strcpy(spi_quest_info->reaction[comment_index - 1], comment);
    }
    return 1;
}

/**
 * Advances to the next request in the list.
 */
static int quest_END(SPI_STACK *arguments, int argument_count) {
    ++spi_quest_info;
    return 1;
}

void CQuestManager::LoadCfg(mgCMemory *stack, char *script, int script_size) {
    spi_questman = this;
    spi_queststack = stack;
    CScriptInterpreter interpreter;
    interpreter.SetTag(quest_cmd_tag);
    interpreter.SetScript(script, script_size);
    interpreter.Run();
}

void CQuestData::Initialize() {
    memset(this, 0, sizeof(*this));
}

void CQuestData::SetQuestFlag(int id, int flag) {
    if (id < 0 || id >= QUEST_PLAY_DATA_MAX) {
        return;
    }
    play[id].accepted = flag;
}

void CQuestData::QuestClear(int id) {
    if (id < 0 || id >= QUEST_PLAY_DATA_MAX) {
        return;
    }
    play[id].cleared = 1;
}

QUEST_PLAY_DATA *CQuestData::GetPlayQuestData(int id) {
    if (id < 0 || id >= QUEST_PLAY_DATA_MAX) {
        return NULL;
    }
    return &play[id];
}

void QuestRequestSetFlag(int id, int flag) {
    CQuestData *quest_data = GetQuestData();
    if (quest_data != NULL) {
        quest_data->SetQuestFlag(id, flag);
    }
}

void QuestRequestClear(int id, int unused) {
    CQuestData *quest_data = GetQuestData();
    if (quest_data != NULL) {
        quest_data->QuestClear(id);
    }
}

int GetQuestRequestStatus(int id) {
    CQuestData *quest_data = GetQuestData();
    if (quest_data == NULL) {
        return QUEST_REQUEST_STATUS_INVALID;
    }
    QUEST_PLAY_DATA *progress = quest_data->GetPlayQuestData(id);
    if (progress == NULL) {
        return QUEST_REQUEST_STATUS_INVALID;
    }
    if (progress->cleared != 0) {
        return QUEST_REQUEST_STATUS_CLEARED;
    }
    return progress->accepted != 0;
}

int CMonsterBook::CountKill(int monster_id, int count) {
    if (monster_id < 0) {
        return 0;
    }
    if (monster_id >= MONSTER_BOOK_ENTRY_MAX) {
        return 0;
    }
    entry[monster_id].kill_count += static_cast<u16>(count);
    if (entry[monster_id].kill_count > MONSTER_BOOK_KILL_MAX) {
        entry[monster_id].kill_count = MONSTER_BOOK_KILL_MAX;
    }
    return entry[monster_id].kill_count;
}
