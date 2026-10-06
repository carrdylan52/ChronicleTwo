#include "common.h"
#include "memcard.hpp"
#include <cstring>
#include <cstdio>

#include "mg_memory.hpp"
#include "mainloop.hpp"
#include "menucommon.hpp"
#include "inventmn.hpp"

static COSBIT_INFO cosbit_table[34] = { /**< Costume item numbers and flag bits. */
    { 111, 0, 0 },
    { 112, 1, 0 },
    { 113, 2, 0 },
    { 114, 3, 0 },
    { 115, 4, 0 },
    { 116, 5, 0 },
    { 117, 6, 0 },
    { 118, 7, 0 },
    { 119, 8, 0 },
    { 120, 9, 0 },
    { 121, 10, 0 },
    { 122, 11, 0 },
    { 123, 12, 0 },
    { 124, 13, 0 },
    { 125, 14, 0 },
    { 126, 15, 0 },
    { 127, 16, 0 },
    { 128, 17, 0 },
    { 129, 18, 0 },
    { 130, 19, 0 },
    { 131, 20, 0 },
    { 132, 21, 0 },
    { 133, 22, 0 },
    { 134, 23, 0 },
    { 258, 24, 0 },
    { 259, 25, 0 },
    { 260, 26, 0 },
    { 261, 27, 0 },
    { 262, 28, 0 },
    { 263, 29, 0 },
    { 264, 30, 0 },
    { 265, 31, 0 },
    { 266, 32, 0 },
    { 267, 33, 0 },
};

static char *MCBrowsetName[3][4] = { /**< Browser titles for each region and save type. */
    {
        "\203_\201[\203N\203N\203\215\203j\203N\203\213",
        "\203_\201[\203N\203N\203\215\203j\203N\203\213\203A\203\213\203o\203\200\203f\201[\203^",
        "\203_\201[\203N\203N\203\215\203j\203N\203\213\202\250\202\334\202\257\203f\201[\203^",
        "\203_\201[\203N\203N\203\215\203j\203N\203\213\201m%s\201n",
    },
    {
        "\202c\202\201\202\222\202\213\201@\202b\202\214\202\217\202\225\202\204\202Q",
        "\202c\202\201\202\222\202\213\201@\202b\202\214\202\217\202\225\202\204\202Q\201@\202`\202\214\202\202\202\225\202\215",
        "\202c\202\201\202\222\202\213\201@\202b\202\214\202\217\202\225\202\204\202Q\201@\202d\202\230\202\224\202\222\202\201",
        "\202c\202\201\202\222\202\213\201@\202b\202\214\202\217\202\225\202\204\202Q\201m%s\201n",
    },
    {
        "\202c\202\201\202\222\202\213\201@\202b\202\210\202\222\202\217\202\216\202\211\202\203\202\214\202\205",
        "\202c\202\201\202\222\202\213\201@\202b\202\210\202\222\202\217\202\216\202\211\202\203\202\214\202\205\201@\202`\202\214\202\202\202\225\202\215",
        "\202c\202\201\202\222\202\213\201@\202b\202\210\202\222\202\217\202\216\202\211\202\203\202\214\202\205\201@\202d\202\230\202\224\202\222\202\201",
        "\202c\202\201\202\222\202\213\201@\202b\202\210\202\222\202\217\202\216\202\211\202\203\202\214\202\205\201m%s\201n",
    },
};

static u16 MCBrowserName_Offset[3][4] = { /**< Second-line offsets of the browser titles. */
    { 32, 16, 16, 32 },
    { 32, 22, 22, 32 },
    { 32, 28, 28, 28 },
};
static u_char omake_file_name[] = "/BESCES-51190dc2omake/BESCES-51190dc2omake"; /**< Bonus data file path. */
static s16 DngTreeSaveFlag; /**< Dungeon tree flag stored with the save. */

// Code (.text)
char *CopyMCBrowserName(int name_no, char *dest, unsigned short *nl_offset) {
    int region = 0;
    if (CheckNowEurope() != 0) {
        region = 2;
    } else if (LanguageCode == 1) {
        region = 1;
    }
    char *result = strcpy(dest, MCBrowsetName[region][name_no]);
    if (nl_offset != NULL) {
        *nl_offset = MCBrowserName_Offset[region][name_no];
    }
    return result;
}

void SetDngTreeFlag(int flag) {
    DngTreeSaveFlag = flag;
}

/**
 * Builds the directory and file path for a save slot.
 */
static void MakeMemoryCardFileName(int slot, char *path) {
    char directory[20] = "/BESCES-51190dkcl%d";
    char file_name[19] = "BESCES-51190dkcl%d";
    sprintf(directory, directory, slot);
    sprintf(file_name, file_name, slot);
    if (path != NULL) {
        strcpy(path, directory);
        strcat(path, "/");
        strcat(path, file_name);
    }
}

/**
 * Builds the directory or file path for the photo album.
 */
static void MakeMemoryCardAlbumName(char *name, int append_again) {
    if (name != NULL) {
        strcpy(name, "/BESCES-51190dc2album");
        if (append_again != 0) {
            strcat(name, "/BESCES-51190dc2album");
        }
    }
}

/**
 * Sums every sixty-fourth byte modulo 255 for a save checksum.
 */
static int MakeCheckDigit(int mode, char *data, int size) {
    int sum = 0;
    if (mode == 0) {
        int blocks = size / 64;
        int i;
        sum = 0;
        for (i = 0; i < blocks; i++) {
            sum += (u8)*data % 255;
            data += 64;
        }
    }
    return sum;
}

CMemoryCardManager::CMemoryCardManager() {
    Initialize(NULL);
}

void CMemoryCardManager::Initialize(mgCMemory *memory) {
    memset(this, 0, sizeof(CMemoryCardManager));
    strcpy(file_name, "BESCES-51190dkcl%d");
    strcpy(game_name, "darkclonicle");
    port = 0;
    file_no = 0;
    fd = -1;
    save_buffer = NULL;
    if (memory != NULL) {
        save_buffer = new ((u_long128 *)memory->Alloc(sizeof(SAVEDATA_FORMAT) / 16 + 2)) SAVEDATA_FORMAT;
        memset(save_buffer, 0, sizeof(SAVEDATA_FORMAT));
    }
    InitError();
    InitSaveFileInfoTable();
    strcpy(version, "dc2Ver4");
    func_no = MC_FUNC_IDLE;
    search_wait = 0x3D;
    step = 0;
    album_buffer_set = 0;
    album_buffer = NULL;
    load_map_no = -1;
    load_program_loop_no = -1;
    load_dng_tree_flag = 0;
    transfer_size = 0;
    transferred = 0;
    total_transferred = 0;
    transfer_result = 0;
    memset(work_buffer, 0, sizeof(work_buffer));
    memset(card, 0, sizeof(card));
    InitPlayDataInfo();
    file_exists = 0;
    memset(icon, 0, sizeof(icon));
    card[0].present = 0;
    card[1].present = 0;
}

void CMemoryCardManager::InitSaveFileInfoTable(void) {
    s32 entry_no;
    s32 entry_offset;
    CMemoryCardManager *entry_base;

    entry_offset = 0;
    entry_no = 0;
    do {
        entry_base = (CMemoryCardManager *)((u8 *)this + entry_offset);
        memset(&entry_base->dir_table[0], 0, sizeof(MC_DIR_ENTRY));
        entry_no += 1;
        entry_base->dir_table[0].name[0] = 0;
        entry_offset += sizeof(MC_DIR_ENTRY);
    } while (entry_no < 17);
}

int CMemoryCardManager::GetOpenAttribute(char *name) {
    for (int i = 0; i < 17; i++) {
        if (strcmp(name, dir_table[i].name) == 0) {
            return 1;
        }
    }
    return 0;
}

void CMemoryCardManager::InitError() {
    memset(&error, 0, sizeof(error));
}

int CMemoryCardManager::InitForMC() {
    int result;
    switch (sceMcInit()) {
        case 0:
            result = 0;
            break;
        case -101:
            result = 1;
            break;
        case -120:
            result = 1;
            break;
        case -121:
            result = 1;
            break;
    }
    return result;
}

int CMemoryCardManager::FinishForMC() {
    return sceMcEnd();
}

void CMemoryCardManager::SetBuff_Album(char *buffer) {
    album_buffer = buffer;
    if (buffer) {
        memset(album_buffer, 0, GetSaveDataSize(MC_SIZE_ALBUM_FILE));
        album_buffer_set = 1;
    }
}

void CMemoryCardManager::SetIconData(MC_ICON_DATA *icon_data, int name_no) {
    memcpy(&icon[0], &icon_data[0], sizeof(MC_ICON_DATA));
    memcpy(&icon[1], &icon_data[1], sizeof(MC_ICON_DATA));
    memcpy(&icon[2], &icon_data[2], sizeof(MC_ICON_DATA));
    sceMcColor bg_colors[4] = {
        { 128, 0, 64, 0 }, { 0, 128, 0, 0 }, { 0, 0, 128, 0 }, { 128, 128, 128, 0 }
    };
    sceMcVu0FVECTOR light_dirs[3] = {
        { 0.5f, 0.5f, 0.5f, 0.0f }, { 0.0f, -0.4f, -0.1f, 0.0f }, { -0.5f, -0.5f, 0.5f, 0.0f }
    };
    sceMcColorF light_colors[3] = {
        { 0.48f, 0.48f, 0.03f, 0.0f }, { 0.5f, 0.33f, 0.2f, 0.0f }, { 0.14f, 0.14f, 0.38f, 0.0f }
    };
    sceMcColorF ambient_color = { 0.5f, 0.5f, 0.5f, 0.0f };
    memset(&icon_sys, 0, sizeof(sceMcIconSys));
    strcpy(icon_sys.head, "PS2D");
    CopyMCBrowserName(name_no, (char *)icon_sys.title_name, &icon_sys.nl_offset);
    icon_sys.trans_rate = 0x60;
    memcpy(icon_sys.bg_color, bg_colors, sizeof(bg_colors[0]));
    memcpy(icon_sys.light_dir, light_dirs, sizeof(light_dirs[0]));
    memcpy(icon_sys.light_color, light_colors, sizeof(light_colors[0]));
    memcpy(icon_sys.ambient, &ambient_color, sizeof(ambient_color));

    strcpy(icon_sys.fname_view, icon[0].name);
    strcpy(icon_sys.fname_copy, icon[1].name);
    strcpy(icon_sys.fname_del, icon[2].name);
}

int CMemoryCardManager::GetIconDataSize() {
    int blocks = (icon[0].size + 0x3FF) / 0x400 + 1;
    blocks += (icon[1].size + 0x3FF) / 0x400;
    return (icon[2].size + 0x3FF) / 0x400 + blocks;
}

s32 CMemoryCardManager::GetSaveDataSize(s32 type) {
    s32 size;

    size = 0;
    if (type == MC_SIZE_SAVE_TOTAL) {
        size = (GetIconDataSize() + 0x19A) << 0xA;
    }
    if (type == MC_SIZE_SAVE_FILE) {
        size = sizeof(SAVEDATA_FORMAT);
    }
    if (type == MC_SIZE_ALBUM_FILE) {
        size = 0x64CB0;
    }
    if (type == MC_SIZE_ICONS) {
        size = GetIconDataSize() << 0xA;
    }
    if (type == MC_SIZE_ALBUM_TOTAL) {
        size = (GetIconDataSize() << 0xA) + 0x654B0;
    }
    if (type == MC_SIZE_SAVE_KB) {
        size = GetIconDataSize() + 0x199;
    }
    if (type == MC_SIZE_UNK_6) {
        size = 0x20800;
    }
    if (type == MC_SIZE_OMAKE_FILE) {
        size = sizeof(CSubGameData);
    }
    if (type == MC_SIZE_OMAKE_TOTAL) {
        size = (GetIconDataSize() << 0xA) + 0x5C70;
    }
    if (type == MC_SIZE_OMAKE_KB) {
        size = GetIconDataSize() + 0x1B;
    }
    return size;
}

void CMemoryCardManager::SetFuncNo(s32 operation) {
    func_no = operation;
    step = 0;
    if (operation == MC_FUNC_IDLE) {
        search_wait = 11;
    }
}

int CMemoryCardManager::GetFuncNo() { return this->func_no; }

u_long CMemoryCardManager::CheckMaxUniqueCounter() {
    u64 max = 0;
    for (int i = 0; i < 13; i++) {
        if (max < file_info[i].unique_counter) {
            max = file_info[i].unique_counter;
        }
    }
    return max;
}

int CMemoryCardManager::GetUpdateFile() {
    u_long max = CheckMaxUniqueCounter();
    int found = -1;
    for (int i = 0; i < 13; i++) {
        if (max == file_info[i].unique_counter) {
            found = i;
            break;
        }
    }
    return found;
}

int CMemoryCardManager::CheckDataFileNum() {
    int count = 0;
    for (int i = 0; i < 13; i++) {
        if (file_info[i].state != 0) {
            count++;
        }
    }
    return count;
}

u32 CMemoryCardManager::CheckOmake(u_long *costume_bit) {
    u32 flags = 0;
    u64 mask = 0;

    for (s64 i = 0; i < 13; i++) {
        if (file_info[i].state == 0) {
            continue;
        }
        flags |= file_info[i].omake_flag;
        mask |= file_info[i].costume_bit;
    }
    if (costume_bit) {
        *costume_bit |= mask;
    }
    return flags;
}

s16 CMemoryCardManager::CheckDebugCode() {
    int code = 0;
    for (int i = 0; i < 13; i++) {
        if (file_info[i].state == 0) {
            continue;
        }
        if (0 < file_info[i].debug_code) {
            code = file_info[i].debug_code;
        }
    }
    return code;
}

void CMemoryCardManager::InitPlayDataInfo() {
    for (int i = 0; i < 13; i++) {
        file_info[i].state = 0;
    }
}

void CMemoryCardManager::UpDateViewInfo(SAVEDATA_INFO *info, SAVEDATA_FORMAT *format) {
    if (info == NULL || format == NULL) {
        return;
    }
    info->program_loop_no = format->program_loop_no;
    info->map_no = format->map_no;
    info->dungeon_no = save_buffer->dungeon_no;
    info->floor_id = save_buffer->floor_id;
    info->play_time = format->save_data.play_time;
    info->progress = format->save_data.game_progress;
    info->debug_code = format->debug_code;
    info->omake_flag = format->omake_flag;
    info->costume_bit = format->costume_bit;
    info->unique_counter = format->unique_counter;
    info->fish_num = format->fish_num;
}

int CMemoryCardManager::Step() {
    int status = 0;
    int changed = 0;
    int job = func_no;
    switch (job) {
        case MC_FUNC_SEARCH_TYPE:
            status = SearchMcType();
            break;
        case MC_FUNC_IDLE:
            search_wait++;
            if (search_wait >= 10) {
                status = SearchMcType();
                search_wait = 0;
            }
            break;
        case MC_FUNC_NONE:
            status = 1;
            break;
        case MC_FUNC_MAKE_DIR:
            status = MakeDir(file_no);
            break;
        case MC_FUNC_GET_ALL_FILE_INFO:
            status = GetAllSaveFileInfo();
            break;
        case MC_FUNC_MAKE_ALBUM_DIR:
            status = MakeDir(-1);
            break;
        case MC_FUNC_SAVE:
            status = SaveToMc(file_no);
            break;
        case MC_FUNC_LOAD:
            status = LoadFromMc(file_no);
            if (status != 0) {
                printf("funcphase : %d\n", step);
            }
            break;
        case MC_FUNC_SAVE_ALBUM:
            status = SaveAlbum();
            break;
        case MC_FUNC_LOAD_ALBUM:
            status = LoadAlbum();
            break;
        case MC_FUNC_CHECK_ALBUM:
            status = CheckAlbum();
            break;
        case MC_FUNC_MAKE_OMAKE_DIR:
            status = MakeDir(-2);
            break;
        case MC_FUNC_FORMAT:
            status = Format();
            break;
        case MC_FUNC_UNFORMAT:
            status = UnFormat();
            break;
        case MC_FUNC_DELETE:
            int delete_index;
            if (error.code == MC_ERROR_NONE) {
                delete_index = file_no;
            } else {
                delete_index = error.file_no;
            }
            status = DeleteFile(delete_index);
            break;
        case MC_FUNC_SAVE_OMAKE:
            status = SaveOamkeFile();
            break;
        case MC_FUNC_LOAD_OMAKE:
            status = LoadOmakeFile();
            break;
        case MC_FUNC_CHECK_OMAKE:
            status = CheckOmakeFile();
            break;
        case MC_FUNC_WRITE_TEST:
            status = Write();
            break;
        case MC_FUNC_CONVERT:
            status = Convert();
            break;
    }
    if (status == 1) {
        step = 0;
        SetFuncNo(MC_FUNC_IDLE);
    } else if (status != 0) {
        McError(status);
    }
    if (job != func_no) {
        changed = 1;
    }
    return changed;
}

char *CMemoryCardManager::GetVersion() {
    return version;
}

int CMemoryCardManager::SearchMcType() {
    static int old_format; /**< Previous format state of the selected card. */
    int command;
    int result;
    MC_CARD_INFO *card;

    if (port == 0 || port == 1) {
        card = &this->card[port];
    } else {
        card = NULL;
    }
    if (card == NULL) {
        return 0;
    }
    if (step == 0) {
        old_format = card->formatted;
    }
    switch (step % 2) {
        case 0:
            if (sceMcSync(1, NULL, NULL) != 0) {
                InitError();
                int status =
                    sceMcGetInfo(port, 1, &card->type, &card->free_size, &card->formatted);
                if (status == 0) {
                    step++;
                } else if (status != -0xC8) {
                    step += 2;
                }
            }
            break;
        case 1:
            if (sceMcSync(1, &command, &result) != 0) {
                card->present = 1;
                card->result = result;
                switch (result) {
                    case 0:
                        break;
                    case -1:
                        card->formatted = 1;
                        break;
                    case -2:
                        card->formatted = 0;
                        break;
                    default:
                        if (result < -10) {
                            card->present = 0;
                        }
                        break;
                }
                step++;
                if (step >= 2) {
                    if (card->present != 0) {
                        if (card->formatted != 0) {
                            if (old_format != 0 && card->formatted != 0) {
                                card->format_change = 0;
                            }
                            if (old_format == 0) {
                                if (card->formatted != 0) {
                                    card->format_change = 1;
                                }
                            }
                            return 1;
                        }
                    }
                }
                if (step >= 10) {
                    if (old_format != 0 && card->formatted != 0) {
                        card->format_change = 0;
                    }
                    if (old_format == 0 && card->formatted == 0) {
                        card->format_change = 0;
                    }
                    if (old_format == 0 && card->formatted != 0) {
                        card->format_change = 1;
                    }
                    if (old_format != 0) {
                        if (card->formatted == 0) {
                            card->format_change = -1;
                        }
                    }
                    return 1;
                }
            }
            break;
    }
    return 0;
}

int CMemoryCardManager::Write() {
    static const u_char test_file[] = "test"; /**< Test output file name. */
    int result;
    int command;
    u8 buffer[0x1000];
    int remaining;
    result = 0;
    sceMcChdir(port, 1, "/", NULL);
    sceMcSync(0, &command, &result);
    sceMcOpen(port, 1, test_file, 0x202);
    sceMcSync(0, &command, &result);
    fd = result;
    remaining = 0x758000;
    do {
        int chunk = 0x1000;
        if (remaining < 0x1000) {
            chunk = remaining;
        }
        sceMcWrite(fd, buffer, chunk);
        sceMcSync(0, &command, &result);
        remaining -= result;
    } while (remaining > 0);
    sceMcFlush(fd);
    sceMcSync(0, &command, &result);
    sceMcClose(fd);
    sceMcSync(0, &command, &result);
    return 1;
}

s32 CMemoryCardManager::Convert(void) {
    return 1;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/memcard", MakeDir__18CMemoryCardManagerFi);
int GetCostumeList(u_long costume_bit, int type, short *list) {
    if (list == NULL) {
        return 0;
    }
    int count = 0;
    COSBIT_INFO *row = cosbit_table;
    u_long bit = 1;
    for (u_long i = 0; i < 0x22; i++) {
        if ((costume_bit & bit) && type == GetItemDataType(row->item_no)) {
            count++;
            *list = row->item_no;
            list++;
        }
        bit <<= 1;
        row++;
    }
    *list = -1;
    return count;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/memcard", SaveToMc__18CMemoryCardManagerFi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/memcard", LoadFromMc__18CMemoryCardManagerFi);
int CMemoryCardManager::SaveAlbum() {
    u_char album_name[0x80];
    int command;
    int result;
    MC_CARD_INFO *card;

    if (port == 0 || port == 1) {
        card = &this->card[port];
    } else {
        card = NULL;
    }
    MC_ERROR_INFO *errors = &error;
    int *album_found = &file_exists;
    switch (step) {
        case 0:
            if (sceMcSync(1, NULL, NULL) != 0) {
                InitError();
                transfer_size = GetSaveDataSize(MC_SIZE_ALBUM_FILE);
                transfer_result = 0;
                transferred = 0;
                total_transferred = 0;
                CDC2AlbumData *album = (CDC2AlbumData *)album_buffer;
                album->check_digit = MakeCheckDigit(0, (char *)album->photo, sizeof(album->photo));
                album->check_digit_copy = MakeCheckDigit(0, (char *)album->photo, sizeof(album->photo));
                write_buffer = album_buffer;
                MakeMemoryCardAlbumName((char *)album_name, 1);
                int opened = sceMcOpen(port, 1, album_name, 0x202);
                step++;
                if (opened != 0) {
                    return -1;
                }
            }
            break;
        case 1:
            if (sceMcSync(1, &command, &result) != 0) {
                if (result < 0) {
                    if (result == -4) {
                        *album_found = 0;
                    } else if (result == -2) {
                        card->formatted = 0;
                    }
                    McError(result);
                    return 1;
                }
                fd = result;
                if (sceMcWrite(fd, write_buffer, 0xC00) == 0) {
                    step++;
                    break;
                }
                errors->code = MC_ERROR_COMMAND;
                return 1;
            }
            break;
        case 2:
            if (sceMcSync(1, &command, &transfer_result) != 0) {
                int written = transfer_result;
                if (written < 0) {
                    if (written == -4) {
                        *album_found = 0;
                    }
                    if (transfer_result == -2) {
                        card->formatted = 0;
                    }
                    McError(transfer_result);
                    return 1;
                }
                transferred += written;
                total_transferred += transfer_result;
                int done = this->transferred;
                int total = transfer_size;
                int chunk;
                int left;

                if (done >= total) {
                    if (sceMcClose(fd) == 0) {
                        step++;
                        break;
                    }
                    return -1;
                }
                left = total - done;
                chunk = 0xC00;
                if (left < 0xC00) {
                    chunk = left;
                }
                sceMcWrite(fd, write_buffer + done, chunk);
            }
            break;
        case 3:
            if (sceMcSync(1, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    return 1;
                }
                step = 4;
            }
            break;
        case 4:
            return 1;
        case 100:
            break;
    }
    return 0;
}

int CMemoryCardManager::LoadAlbum() {
    char album_name[0x80];
    int command;
    int result;
    MC_ERROR_INFO *errors = &error;

    switch (step) {
        case 0:
            if (sceMcSync(1, NULL, NULL) != 0) {
                InitError();
                transfer_size = GetSaveDataSize(MC_SIZE_ALBUM_FILE);
                memset(album_buffer, 0, transfer_size);
                transfer_result = 0;
                transferred = 0;
                total_transferred = 0;
                read_buffer = album_buffer;
                MakeMemoryCardAlbumName(album_name, 1);
                int opened = sceMcOpen(port, 1, (const u_char *)album_name, 1);
                step++;
                if (opened != 0) {
                    return -1;
                }
            }
            break;
        case 1:
            if (sceMcSync(1, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    return 1;
                }
                fd = result;
                if (sceMcRead(fd, read_buffer, 0x1000) == 0) {
                    step++;
                    break;
                }
                return 1;
            }
            break;
        case 2:
            if (sceMcSync(1, &command, &transfer_result) != 0) {
                int transferred = transfer_result;
                if (transferred <= 0) {
                    McError(transferred);
                    if (this->transferred < transfer_size) {
                        errors->code = MC_ERROR_BROKEN;
                        return 1;
                    }
                    return 1;
                }
                this->transferred += transferred;
                total_transferred += transfer_result;
                int done = this->transferred;
                int total = transfer_size;
                int chunk;
                int left;

                if (done >= total) {
                    if (sceMcClose(fd) == 0) {
                        step++;
                        break;
                    }
                    errors->code = MC_ERROR_BROKEN;
                    return 1;
                }
                left = total - done;
                chunk = 0x1000;
                if (left < 0x1000) {
                    chunk = left;
                }
                sceMcRead(fd, read_buffer + done, chunk);
            }
            break;
        case 3:
            if (sceMcSync(1, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    return 1;
                }
                if (this->transferred < transfer_size) {
                    errors->code = MC_ERROR_BROKEN;
                }
                CDC2AlbumData *album = (CDC2AlbumData *)album_buffer;
                int stored_digit = album->check_digit;
                if (stored_digit != 0 || album->check_digit_copy != 0) {
                    if (stored_digit !=
                        MakeCheckDigit(0, (char *)album->photo, sizeof(album->photo))) {
                        errors->code = MC_ERROR_BROKEN;
                    }
                }
                return 1;
            }
            break;
    }
    return 0;
}

int CMemoryCardManager::CheckAlbum() {
    char album_name[0x80];
    int command;
    int result;
    int *album_found = &file_exists;
    MC_ERROR_INFO *error_record = &error;
    switch (step) {
        case 0:
            if (sceMcSync(1, NULL, NULL) != 0) {
                InitSaveFileInfoTable();
                MakeMemoryCardAlbumName(album_name, 1);
                int status = sceMcGetDir(port, 1, album_name, 0, 0x11, dir_table);
                *album_found = 0;
                if (status == 0) {
                    step = 1;
                } else {
                    error_record->code = MC_ERROR_NONE;
                    return 1;
                }
            }
            break;
        case 1:
            if (sceMcSync(1, &command, &result) != 0) {
                dir_entries = 0;
                *album_found = 0;
                if (result >= 0) {
                    error_record->code = MC_ERROR_NONE;
                    if (result > 0) {
                        *album_found = 1;
                    }
                    dir_entries = result;
                    u_int file_size = dir_table[0].file_size;
                    if (file_size < GetSaveDataSize(MC_SIZE_ALBUM_FILE)) {
                        error_record->code = MC_ERROR_BROKEN;
                        *album_found = 0;
                    }
                    strlen(dir_table[0].name);
                    return 1;
                }
                McError(result);
                if (result == -2) {
                    error_record->code = MC_ERROR_UNFORMATTED;
                } else if (result == -4) {
                    error_record->code = MC_ERROR_NONE;
                    *album_found = 0;
                    dir_entries = result;
                    return 1;
                }
                error_record->func_no = GetFuncNo();
                error_record->step = step;
                return 1;
            }
            break;
    }
    return 0;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/memcard", SaveOamkeFile__18CMemoryCardManagerFv);
int CMemoryCardManager::LoadOmakeFile() {
    int result;
    int command;
    MC_CARD_INFO *card;

    if (port == 0 || port == 1) {
        card = &this->card[port];
    } else {
        card = NULL;
    }
    switch (step) {
        case 0:
            if (sceMcSync(1, NULL, NULL) != 0) {
                if (McCheckMCPs2(card) == 0) {
                    return 1;
                }
                InitError();
                CSubGameData *sub_game = GetSubGameSaveData();
                if (sub_game == NULL) {
                    return 1;
                }
                sub_game->Initialize();
                transfer_size = sizeof(CSubGameData);
                memset(sub_game, 0, transfer_size);
                transfer_result = 0;
                transferred = 0;
                total_transferred = 0;
                read_buffer = (char *)sub_game;
                int opened = sceMcOpen(port, 1, omake_file_name, 1);
                step++;
                if (opened != 0 && opened != -0xC8) {
                    error.code = MC_ERROR_COMMAND;
                    return 1;
                }
            }
            break;
        case 1:
            if (sceMcSync(0, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    return 1;
                }
                fd = result;
                if (sceMcRead(fd, read_buffer, 0x1000) == 0) {
                    step++;
                    break;
                }
                error.code = MC_ERROR_LOAD;
                return 1;
            }
            break;
        case 2:
            if (sceMcSync(1, &command, &transfer_result) != 0) {
                int transferred = transfer_result;
                if (transferred < 0) {
                    McError(transferred);
                    error.code = MC_ERROR_LOAD;
                    return 1;
                }
                this->transferred += transferred;
                total_transferred += transfer_result;
                int done = this->transferred;
                if (done >= transfer_size) {
                    if (sceMcClose(fd) == 0) {
                        step++;
                        break;
                    }
                    error.code = MC_ERROR_LOAD;
                    return 1;
                }
                sceMcRead(fd, read_buffer + done, 0x1000);
            }
            break;
        case 3:
            if (sceMcSync(0, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    return 1;
                }
                int incomplete = 0;
                if (transferred != transfer_size) {
                    incomplete = 1;
                }
                if (incomplete != 0) {
                    error.code = MC_ERROR_LOAD;
                    return 1;
                }
                return 1;
            }
            break;
    }
    return 0;
}
#ifdef NONMATCHING
int CMemoryCardManager::CheckOmakeFile() {
    int command;
    int result;
    int &omake_found = file_exists;
    MC_ERROR_INFO *error_record = &error;

    switch (step) {
        case 0:
            if (sceMcSync(1, NULL, NULL) != 0) {
                InitSaveFileInfoTable();
                omake_found = 0;
                if (sceMcGetDir(port, 1, "/BESCES-51190dc2omake/*", 0, 0xD, dir_table) == 0) {
                    step = 1;
                } else {
                    error_record->code = MC_ERROR_NONE;
                    return 1;
                }
            }
            break;
        case 1:
            if (sceMcSync(1, &command, &result) != 0) {
                dir_entries = 0;
                if (result >= 7) {
                    error_record->code = MC_ERROR_NONE;
                    omake_found = 1;
                    omake_flag = 0;
                    dir_entries = result;
                    u_int file_size = dir_table[6].file_size;
                    if (file_size < GetSaveDataSize(MC_SIZE_OMAKE_FILE)) {
                        error_record->code = MC_ERROR_BROKEN;
                        omake_found = 0;
                    }
                    for (int i = 0; i < result; i++) {
                        strlen(dir_table[i].name);
                    }
                    step = 2;
                    sceMcOpen(port, 1, omake_file_name, 1);
                    break;
                }
                if (result < 0) {
                    McError(result);
                    if (result == -2) {
                        error_record->code = MC_ERROR_UNFORMATTED;
                    } else if (result == -4) {
                        error_record->code = MC_ERROR_NONE;
                        dir_entries = result;
                        return 1;
                    }
                    error_record->func_no = GetFuncNo();
                    error_record->step = step;
                    return 1;
                }
                error_record->code = MC_ERROR_FILE;
                omake_found = 2;
                return 1;
            }
            break;
        case 2:
            if (sceMcSync(1, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    return 1;
                }
                fd = result;
                sceMcRead(fd, work_buffer, 0x100);
                if (sceMcSync(0, &command, &result) != 0) {
                    if (result < 0) {
                        McError(result);
                        return 1;
                    }
                    sceMcClose(fd);
                    step++;
                }
            }
            break;
        case 3:
            if (sceMcSync(1, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    return 1;
                }
                CSubGameData sub_game;
                memcpy(&sub_game, work_buffer, 0x100);
                omake_flag = sub_game.play_enable;
                return 1;
            }
            break;
    }
    return 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/memcard", CheckOmakeFile__18CMemoryCardManagerFv);
#endif
int CMemoryCardManager::Format() {
    int command;
    int result;
    MC_CARD_INFO *record;
    MC_ERROR_INFO *error_record;
    int current_port = port;
    if (current_port == 0 || current_port == 1) {
        record = &card[current_port];
    } else {
        record = NULL;
    }
    error_record = &error;
    switch (step) {
        case 0:
            if (sceMcSync(1, NULL, NULL) != 0) {
                if (sceMcFormat(port, 1) == 0) {
                    step += 1;
                } else {
                    error_record->code = MC_ERROR_COMMAND;
                    return 1;
                }
            }
            break;
        case 1:
            if (sceMcSync(1, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    return 1;
                }
                step += 1;
            }
            break;
        case 2:
            if (sceMcGetInfo(port, 1, &record->type, &record->free_size, &record->formatted) ==
                0) {
                step += 1;
            } else {
                error_record->code = MC_ERROR_COMMAND;
                return 1;
            }
            break;
        case 3:
            if (sceMcSync(1, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    if (result < -9) {
                        record->present = 0;
                    }
                    return 1;
                }
                record->present = 1;
                return 1;
            }
            break;
    }
    return 0;
}

int CMemoryCardManager::DeleteFile(int file_no) {
    int result;
    int command;
    switch (step) {
        case 0:
            if (sceMcSync(1, NULL, NULL) != 0) {
                char name[64] = "darkcloud%d";
                sprintf(name, name, file_no);
                if (sceMcDelete(port, 1, name) == 0) {
                    step += 1;
                } else {
                    sceMcSync(1, NULL, NULL);
                }
            }
            break;
        case 1:
            if (sceMcSync(1, &command, &result) != 0 && command == 0xF) {
                if (result < 0) {
                    error.retry_count += 1;
                    if (error.retry_count > 0x78) {
                        return 1;
                    }
                    break;
                }
                return 1;
            }
            break;
    }
    return 0;
}

int CMemoryCardManager::McError(int result) {
    MC_CARD_INFO *record;
    MC_ERROR_INFO *error_record = &error;
    if (port == 0 || port == 1) {
        record = &card[port];
    } else {
        record = NULL;
    }
    switch (result) {
        case -2:
        case -12:
            error_record->code = MC_ERROR_UNFORMATTED;
            record->formatted = 0;
            break;
        case -3:
            error_record->code = MC_ERROR_FULL;
            break;
        case -4:
        case -5:
            break;
        case -8:
            break;
    }
    if (result < -10) {
        error_record->code = MC_ERROR_NO_CARD;
        record->present = 0;
        if (func_no == MC_FUNC_SAVE) {
            error_record->code = MC_ERROR_FILE;
        }
    }
    if (result < 0) {
        error_record->func_no = GetFuncNo();
        error_record->step = step;
        error_record->file_no = file_no;
    }
    return 1;
}

int CMemoryCardManager::UnFormat() {
    int status;
    int command;
    int result;
    switch (step) {
        case 0:
            status = sceMcUnformat(port, 1);
            if (status == 0) {
                step += 1;
            } else {
                sceMcSync(1, &status, &result);
            }
            break;
        case 1:
            status = sceMcSync(1, &command, &result);
            if (status != 0 && command == 0x11 && result >= 0) {
                card[port].formatted = 0;
                return 1;
            }
            break;
    }
    return 0;
}

int CMemoryCardManager::GetSaveFileInfoFromMc(int file_no, int *step) {
    char save_name[0x20];
    char file_name[0x80];
    int command;
    int result;
    SAVEDATA_INFO *slot = &file_info[file_no];

    int phase = *step;
    switch (phase % 4) {
        case 0:
            if (sceMcSync(1, NULL, NULL) != 0) {
                memset(slot, 0, sizeof(SAVEDATA_INFO));
                sprintf(save_name, "BESCES-51190dkcl%d", file_no);
                if (GetOpenAttribute(save_name) == 0) {
                    *step += 4;
                    return -1;
                }
                MakeMemoryCardFileName(file_no, file_name);
                if (sceMcOpen(port, 1, (const u_char *)file_name, 1) == 0) {
                    *step += 1;
                    break;
                }
                *step += 4;
                return 1;
            }
            break;
        case 1:
            if (sceMcSync(1, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    *step += 3;
                    return 1;
                }
                fd = result;
                transferred = 0;
                transfer_result = 0;
                transfer_size = 0x2800;
                error.retry_count = 0;
                memset(save_buffer, 0, transfer_size);
                if (sceMcRead(fd, save_buffer, transfer_size) == 0) {
                    *step += 1;
                    break;
                }
                McError(result);
                *step += 3;
                return 1;
            }
            break;
        case 2:
            if (sceMcSync(1, &command, &transfer_result) != 0) {
                int transferred = transfer_result;
                if (transferred < 0) {
                    McError(transferred);
                    *step += 2;
                    return 1;
                }
                this->transferred += transferred;
                total_transferred += transfer_result;
                if (this->transferred < transfer_size) {
                    if (sceMcClose(fd) == 0) {
                        *step += 1;
                    } else {
                        *step += 2;
                        return -1;
                    }
                }
                if (this->transferred >= transfer_size) {
                    if (sceMcClose(fd) == 0) {
                        *step += 1;
                        break;
                    }
                    *step += 2;
                    return -1;
                }
            }
            break;
        case 3:
            if (sceMcSync(1, &command, &result) != 0) {
                if (result < 0) {
                    McError(result);
                    *step += 1;
                    return -1;
                }
                MC_ERROR_INFO *error_record = &error;
                if (this->transferred < transfer_size) {
                    slot->state = 0;
                    error_record->code = MC_ERROR_BROKEN;
                    error_record->file_no = this->file_no;
                    *step += 1;
                    return -1;
                }
                int broken = 0;
                if (strcmp(save_buffer->version, "dc2Ver4") != 0) {
                    broken = 1;
                }
                if (broken != 0) {
                    printf("differnt version\n");
                }
                if ((s8)save_buffer->incomplete != 0) {
                    broken = 1;
                }
                if (slot != NULL) {
                    slot->state = 0;
                    if (broken == 0) {
                        slot->state = 1;
                        slot->file_no = file_no;
                        UpDateViewInfo(slot, save_buffer);
                    }
                }
                *step += 1;
                return 1;
            }
            break;
    }
    return 0;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/memcard", GetAllSaveFileInfo__18CMemoryCardManagerFv);
int McCheckMCPs2(MC_CARD_INFO *card) {
    if (card == NULL) {
        return 0;
    }
    if (card->present == 0 || card->type != 2) {
        return 0;
    }
    return 1;
}

int McCheckMCPs2Boot(MC_CARD_INFO *card, int size) {
    if (card == NULL) {
        return 0;
    }
    if (card->present == 0 || card->type != 2) {
        return 0;
    }
    if (card->formatted != 0 && card->free_size <= size) {
        return 0;
    }
    return 1;
}

COSBIT_INFO *GetCosInfo(int item_no) {
    COSBIT_INFO *row = cosbit_table;
    for (int i = 0; i < 0x22; i++) {
        if (row->item_no == item_no) {
            return row;
        }
        row++;
    }
    return NULL;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", cosbit_table__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", MCBrowserName_Offset__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_838__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_839__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1031__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1032__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1033__8__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1034__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_2131__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_2297__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_843__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_852__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_922__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_923__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_924__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1036__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1229__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1230__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1315__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1453__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1454__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1455__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1456__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1581__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1582__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1679__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1680__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1681__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1953__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_1954__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_2083__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", at_2285__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/memcard", NowProgramLoopNo__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(DngTreeSaveFlag, 0x4);
INCLUDE_BSS(old_format_1242, 0x4);
INCLUDE_BSS(iconNo_1323, 0x4);
INCLUDE_BSS(init_1324, 0x4);
INCLUDE_BSS(test_write_num_1476, 0x4);
INCLUDE_BSS(init_1477, 0x4);
INCLUDE_BSS(SubGameOmakeTempBuffer, 0x4);
INCLUDE_BSS(ReadFileNo_2290, 0x4);
INCLUDE_BSS(init_2291, 0x4);
