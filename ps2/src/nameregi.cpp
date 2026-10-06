#include "common.h"
#include "nameregi.hpp"
#include "menuaqua.hpp"
#include "mglib.hpp"
#include "menumain.hpp"
#include "font.hpp"
#include "menucls1.hpp"
#include "mg_drawprim.hpp"
#include "mg_texture.hpp"
#include "mg_memory.hpp"
#include "mainloop.hpp"
#include "menucommon.hpp"
#include "menudraw.hpp"
#include "drawwin.hpp"
#include "nd_meswin.hpp"
#include "mg_math.hpp"
#include <cstring>
#include <cmath>
#include "userdata.hpp"
#include "password.hpp"
#include "gamedata.hpp"

/**
 * Topic of the keyword-entry prompt.
 */
static char NameRegiTopic[0x40];
/**
 * Keyword-entry result code.
 */
static s8 NameRegiCode;
/**
 * Character sets offered for each language.
 */
static s8 NameStrSelectModeTable[7][6] = {
    {2, 1, 0, 4, 3, -1},
    {-1, -1, 0, 4, -1, -1},
    {-1, -1, 0, 4, -1, -1},
    {-1, -1, 0, 4, -1, -1},
    {-1, -1, 0, 4, -1, -1},
    {-1, -1, 0, 4, -1, -1},
    {-1, -1, 0, 4, -1, -1}
};
/**
 * Kanji ranges and their available font characters.
 */
static NAMEREGI_KANJI_INDEX NameRegiSearchKanjiIndexTable[0x2C] = {
    {{-120, -97}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-120, -56}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-119, 69}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-119, 97}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-119, -104}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-119, -70}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-118, -23}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-117, -29}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-116, 84}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-116, -63}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-115, -79}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-114, 100}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-112, 121}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-112, -94}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-111, 88}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-111, -68}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-110, 108}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-110, -61}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-110, -32}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-109, 101}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-109, -34}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-109, -15}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-108, 71}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-108, 75}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-108, 84}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-108, 98}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-108, -39}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-107, 115}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-107, -72}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-107, -37}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-106, -128}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-106, -95}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-106, -79}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-106, -69}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-106, -52}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-106, -25}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-106, -7}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-105, 92}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-105, -123}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-105, -104}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-105, -38}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-105, -33}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-104, 67}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}},
    {{-104, 96}, {0, 0}, 0, {0, 0}, NULL, {0, 0, 0, 0}}
};
/**
 * Reading headings of the kanji grid.
 */
static s8 testchar[0x2C][2] = {
    {-126, -96},
    {-126, -94},
    {-126, -92},
    {-126, -90},
    {-126, -88},
    {-126, -87},
    {-126, -85},
    {-126, -83},
    {-126, -81},
    {-126, -79},
    {-126, -77},
    {-126, -75},
    {-126, -73},
    {-126, -71},
    {-126, -69},
    {-126, -67},
    {-126, -65},
    {-126, -62},
    {-126, -60},
    {-126, -58},
    {-126, -56},
    {-126, -55},
    {-126, -54},
    {-126, -53},
    {-126, -52},
    {-126, -51},
    {-126, -48},
    {-126, -45},
    {-126, -42},
    {-126, -39},
    {-126, -36},
    {-126, -35},
    {-126, -34},
    {-126, -33},
    {-126, -32},
    {-126, -30},
    {-126, -28},
    {-126, -26},
    {-126, -25},
    {-126, -24},
    {-126, -23},
    {-126, -22},
    {-126, -21},
    {-126, -19}
};
/**
 * ASCII character conversion table.
 */
static s8 txt_table[0x3B] = {
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 97, 98,
    99, 100, 101, 102, 103, 104, 105, 106, 107, 109, 110, 112,
    113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 65, 66,
    67, 68, 69, 70, 71, 72, 74, 75, 76, 77, 78, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 0
};
/**
 * Active name entry menu.
 */
static CNameRegiMenu *NameRegiMenuPtr;
/**
 * Most recently loaded texture block.
 */
static int OldReloadTexNumber;
/**
 * Columns in each character grid.
 */
static s8 NameRegistGyouLimmitTable[5] = {13, 15, 15, 19, 15};
/**
 * Texture of the name-entry mark cursor.
 */
static mgCTexture *NameRegiCursor;
/**
 * Texture used for the name-entry frame.
 */
static mgCTexture *NameRegiTex1;
/**
 * Maximum entered name length.
 */
static s16 NameRegistMax = 10;
/**
 * ASCII characters corresponding to the menu Shift-JIS grid.
 */
static char ascii_code_table[95] = {
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
    77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
    89, 90, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
    119, 120, 121, 122, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 32, 32, 32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 32, 47, 58, 59, 60,
    61, 62, 63, 64, 91, 93, 95, 123, 125, 124, 0
};
/**
 * Shift-JIS character conversion table.
 */
static s8 txt_table2[0x3B][2] = {
    {-126, 79},
    {-126, 80},
    {-126, 81},
    {-126, 82},
    {-126, 83},
    {-126, 84},
    {-126, 85},
    {-126, 86},
    {-126, 87},
    {-126, 88},
    {-126, -127},
    {-126, -126},
    {-126, -125},
    {-126, -124},
    {-126, -123},
    {-126, -122},
    {-126, -121},
    {-126, -120},
    {-126, -119},
    {-126, -118},
    {-126, -117},
    {-126, -115},
    {-126, -114},
    {-126, -112},
    {-126, -111},
    {-126, -110},
    {-126, -109},
    {-126, -108},
    {-126, -107},
    {-126, -106},
    {-126, -105},
    {-126, -104},
    {-126, -103},
    {-126, -102},
    {-126, 96},
    {-126, 97},
    {-126, 98},
    {-126, 99},
    {-126, 100},
    {-126, 101},
    {-126, 102},
    {-126, 103},
    {-126, 105},
    {-126, 106},
    {-126, 107},
    {-126, 108},
    {-126, 109},
    {-126, 111},
    {-126, 112},
    {-126, 113},
    {-126, 114},
    {-126, 115},
    {-126, 116},
    {-126, 117},
    {-126, 118},
    {-126, 119},
    {-126, 120},
    {-126, 121},
    {0, 0}
};
/**
 * Default sphida names by language.
 */
static char *Sfida_default_Name[7] = {"\203\206\203\212\203X", "Max", "Max", "Max", "Max", "Max", "Max"};

static int search_txt_jis(char *text);
static int search_txt_asci(char *text);
static int nameregist_local_key(MENU_SELECT_PARAM *param, int &keys, s16 *step, int table_index);

// Check division by a runtime grid width.
#pragma divbyzerocheck on

// Code (.text)
void SetEventKeyword(char *keyword, char *topic, int code) {
    Nameregi_Target.keyword[0] = 0;
    Nameregi_Target.keyword[1] = 0;
    if (keyword != NULL) {
        strcpy(Nameregi_Target.keyword, keyword);
    }
    NameRegiTopic[0] = 0;
    NameRegiTopic[1] = 0;
    if (topic != NULL) {
        strcpy(NameRegiTopic, topic);
    }
    NameRegiCode = code;
}

int CheckDeleteNameRegisteItem(CGameDataUsed *item) {
    if (item == NULL) {
        return 0;
    }
    if (item->used_type == USED_ITEM_TYPE_WEAPON || item->used_type == USED_ITEM_TYPE_ROBO_PART) {
        return 1;
    }
    return 0;
}

int CNameRegiMenu::GetActiveFontMode() {
    int language = LanguageCode;
    if (language < 0) {
        language = 0;
    }
    if (language > 1) {
        language = 1;
    }
    return NameStrSelectModeTable[language][select_mode];
}

void CNameRegiMenu::CopyAsciiToJis(char *src, char *dst) {
    if (src == NULL || dst == NULL) {
        return;
    }
    if (CheckNowEurope() != 0) {
        strcpy(dst, src);
    } else {
        char *ascii_codes = ascii_code_table;
        while ((s8)*src != 0) {
            s64 character = (s8)*src;
            int matched_index = -1;
            int table_index = 0;
            while ((s8)ascii_codes[table_index] != 0) {
                if (character == (s64)(s8)ascii_codes[table_index]) {
                    matched_index = table_index;
                    break;
                }
                table_index++;
            }
            if (0 <= matched_index) {
                dst[0] = jis_table[matched_index * 2];
                dst[1] = jis_table[matched_index * 2 + 1];
                dst += 2;
            }
            src++;
        }
        *dst = 0;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/nameregi", CopyJisToAscii__13CNameRegiMenuFPcPc);
int CheckChronicleKanjiFont(mgCMemory *stack) {
    char name[3];
    int total;
    int row;
    if (stack == NULL) {
        return 0;
    }
    name[2] = 0;
    total = 0;
    row = 0;
    do {
        NAMEREGI_KANJI_INDEX *current;
        NAMEREGI_KANJI_NODE *node;
        NAMEREGI_KANJI_NODE *previous;
        u8 high;
        u8 low;
        int i;
        NAMEREGI_KANJI_INDEX *next;
        int count;
        node = NULL;
        previous = NULL;
        count = 0;
        i = 0;
        current = &NameRegiSearchKanjiIndexTable[row];
        next = &NameRegiSearchKanjiIndexTable[row + 1];
        current->list = NULL;
        high = current->code[0];
        low = current->code[1];
        do {
            name[0] = high;
            name[1] = low;
            if (0 <= GetFontNo(name)) {
                if (current->list == NULL) {
                    current->list = (NAMEREGI_KANJI_NODE *)stack->Alloc(1);
                    node = current->list;
                } else {
                    previous->next = (NAMEREGI_KANJI_NODE *)stack->Alloc(1);
                    node = previous->next;
                }
                node->code[0] = high;
                node->code[1] = low;
                count++;
                node->next = NULL;
            }
            if (low < 0xFF) {
                low++;
            } else {
                low = 0;
                high++;
            }
            previous = node;
            if (next->code[0] == high && next->code[1] == low) {
                break;
            }
            i++;
        } while (i < 0x200);
        if (node != NULL) {
            node->next = NULL;
        }
        current->num = count;
        total += count;
        row++;
    } while (row < 0x2C);
    return total;
}

int GetNameRegistFontKanjiList(int cell, char *dst) {
    s8 *signed_dst = (s8 *)dst;
    int position = 0;
    int row = 0;
    NAMEREGI_KANJI_NODE *node;
    int table_index = 0;
    do {
        if (position == cell) {
            dst[0] = testchar[row][0];
            dst[1] = testchar[row][1];
            return 1;
        }
        node = NameRegiSearchKanjiIndexTable[table_index].list;
        position++;
        if (node != NULL) {
            do {
                if (position == cell) {
                    dst[0] = node->code[0];
                    dst[1] = node->code[1];
                    return 0;
                }
                node = node->next;
                position++;
            } while (node != NULL);
        }
        if (position == cell) {
            signed_dst[0] = -0x7F;
            signed_dst[1] = 0x40;
            return 2;
        }
        row++;
        position++;
        table_index++;
    } while (row < 0x2C);
    return -1;
}

/**
 * Sizes and centers the message window frame.
 */
void AdjustWaku(CDC2Mes *mes, RECT *waku) {
    mes->StepMsg();
    int width = mes->line_w[0];
    int position[2] = {0, 36};
    position[0] = (mgScreenWidth - width) >> 1;
    mes->SetPutPos(position);
    waku->x = position[0] - 20;
    waku->y = position[1] - 22;
    waku->width = width + 44;
    waku->height = mes->font_h + 36;
}

/**
 * Finds a Shift-JIS character in the conversion table.
 */
static int search_txt_jis(char *text) {
    int index = 0;
    do {
        if ((s8)text[0] == txt_table2[index][0] && (s8)text[1] == txt_table2[index][1]) {
            return index;
        }
        index++;
    } while (index < 0x3A);
    return -1;
}

/**
 * Finds an ASCII character in the conversion table.
 */
static int search_txt_asci(char *text) {
    s8 *character = (s8 *)text;
    int table_index = 0;
    do {
        if (*character == txt_table[table_index]) {
            return table_index;
        }
        table_index++;
    } while (table_index < 0x3A);
    return -1;
}

void ConvertShitJiss2Ascii(char *src, char *dst) {
    if (src == NULL || dst == NULL) {
        return;
    }
    if (CheckNowEurope() != 0) {
        strcpy(dst, src);
        return;
    }
    while ((s8)*src != 0) {
        int index = search_txt_jis(src);
        if (0 <= index) {
            *dst = txt_table[index];
        }
        src += 2;
        dst++;
    }
}

void ConvertAscii2ShitJiss(char *src, char *dst) {
    if (dst == NULL || src == NULL) {
        return;
    }
    char *input = src;
    char *output = dst;
    while ((s8)*input != 0) {
        int index = search_txt_asci(input);
        if (0 <= index) {
            output[0] = txt_table2[index][0];
            output[1] = txt_table2[index][1];
        }
        input++;
        output += 2;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/nameregi", NameRegistInit__FP9mgCMemoryPii);
int NameRegistKey() {
    return NameRegiMenuPtr->KeyStep();
}

void NameRegistDraw() {
    OldReloadTexNumber = -1;
    NameRegiMenuPtr->DrawBaseBoard();
    NameRegiMenuPtr->DrawSelectedWord();
    NameRegiMenuPtr->DrawActiveFont();
    NameRegiMenuPtr->DrawMarkCursor();
    NameRegiMenuPtr->DrawMessage();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/nameregi", CheckInputWord__FPc);
/**
 * Moves a character-grid cursor and adjusts the navigation keys.
 */
static int nameregist_local_key(MENU_SELECT_PARAM *param, int &keys, s16 *step, int table_index) {
    static s16 limits[5] = {65, 90, 90, 114, 90};
    int direction = 0;
    if (keys & 1) {
        direction = 1;
        param->pos += step[0];
    }
    if (keys & 2) {
        direction = 2;
        param->pos += step[1];
    }
    if (keys & 4) {
        s16 row_size = step[1];
        if (param->pos % row_size == 0) {
            param->pos += row_size - 1;
        } else {
            param->pos += step[2];
        }
        direction = 4;
    } else if (keys & 8) {
        s16 row_size = step[1];
        int last = row_size - 1;
        int same = last == param->pos % row_size;
        if (same) {
            param->pos -= last;
        } else {
            param->pos += step[3];
        }
        direction = 1;
    }
    if (keys & 0x10 || keys & 0x40) {
        keys = 4;
    }
    if (keys & 0x20 || keys & 0x80) {
        keys = 8;
    }
    int base = param->pos;
    if (base < 0) {
        param->pos = base + limits[table_index];
        direction = -1;
    }
    if (limits[table_index] <= param->pos) {
        param->pos += step[0];
        keys &= ~2;
        keys |= 1;
    }
    return direction;
}

void CNameRegiMenu::ConvertPositionNameRegi(int to_command) {
    static s8 column_bounds[2][5][8] = {
        {
            {0, 1, 3, 5, 8, 11, 12, 0},
            {1, 2, 5, 7, 9, 11, 13, 0},
            {1, 2, 5, 7, 9, 11, 13, 0},
            {1, 3, 5, 7, 11, 15, 20, 0},
            {0, 1, 2, 3, 5, 7, 9, 14}
        },
        {
            {0, 1, 3, 5, 8, 11, 12, 0},
            {1, 2, 5, 7, 9, 11, 13, 0},
            {1, 2, 5, 7, 9, 11, 13, 0},
            {1, 3, 5, 7, 11, 15, 20, 0},
            {0, 2, 5, 5, 9, 12, 9, 14}
        }
    };
    int font_mode = GetActiveFontMode();
    if (to_command == 0) {
        int col = command_pos;
        s8 table[3][5][12] = {
            {
                {0, 0, 7, 9, 11, 0, 0, 2, 4, 6, 8, 12},
                {0, 1, 3, 8, 10, 0, 1, 3, 5, 7, 9, 12},
                {0, 1, 3, 8, 10, 0, 1, 3, 5, 7, 9, 12},
                {0, 0, 9, 10, 11, 0, 1, 3, 6, 8, 12, 15},
                {0, 0, 8, 10, 11, 0, 0, 2, 4, 7, 9, 14}
            },
            {
                {0, 0, 5, 8, 0, 0, 0, 9, 1, 5, 8, 12},
                {0, 1, 3, 8, 10, 0, 1, 3, 5, 7, 9, 12},
                {0, 1, 3, 8, 10, 0, 1, 3, 5, 7, 9, 12},
                {0, 0, 9, 10, 11, 0, 1, 3, 6, 8, 12, 15},
                {0, 0, 8, 10, 11, 0, 0, 9, 2, 6, 9, 13}
            },
            {
                {0, 0, 7, 9, 11, 0, 0, 2, 4, 6, 8, 12},
                {0, 1, 3, 8, 10, 0, 1, 3, 5, 7, 9, 12},
                {0, 1, 3, 8, 10, 0, 1, 3, 5, 7, 9, 12},
                {0, 0, 9, 10, 11, 0, 1, 3, 6, 8, 12, 15},
                {0, 0, 8, 10, 11, 0, 0, 11, 2, 6, 9, 13}
            }
        };
        int language = LanguageCode;
        if (language > 0) {
            language = 1;
        }
        select.pos = table[language][font_mode][col];
    }
    if (to_command == 1) {
        int remainder = select.pos % NameRegistGyouLimmitTable[font_mode];
        int language = 0;
        if (LanguageCode > 0) {
            language = 1;
        }
        s8 *limit = column_bounds[language][font_mode];
        if (remainder < limit[0]) {
            command_pos = 5;
        } else if (remainder < limit[1]) {
            command_pos = 6;
        } else if (remainder < limit[2]) {
            command_pos = 7;
            if (LanguageCode > 0) {
                command_pos = 8;
            }
        } else if (remainder < limit[3]) {
            command_pos = 8;
        } else if (remainder < limit[4]) {
            command_pos = 9;
        } else if (remainder < limit[5]) {
            command_pos = 0xA;
            if (LanguageCode > 0) {
                command_pos = 7;
            }
        } else {
            command_pos = 0xB;
        }
    }
}

int CNameRegiMenu::CheckKanjiPosition(int key, short *step, int limit_no) {
    MENU_SELECT_PARAM *param = &select;
    int result = 0;
    char cell[4];
    cell[2] = 0;
    int kind = GetNameRegistFontKanjiList(select.pos + select.row * 0x13, cell);
    while (kind != 0 && kind != 1) {
        result = nameregist_local_key(param, key, step, limit_no);
        if (result == -1) {
            break;
        }
        kind = GetNameRegistFontKanjiList(param->pos + param->row * 0x13, cell);
        if (kind < 0) {
            key = 1;
            while (kind != 0 && kind != 1) {
                result = nameregist_local_key(param, key, step, limit_no);
                if (result == -1) {
                    break;
                }
                kind = GetNameRegistFontKanjiList(param->pos + param->row * 0x13, cell);
            }
            break;
        }
    }
    return result;
}

int CNameRegiMenu::KeyStep() {
    static s8 character_buttons[4] = {2, 1, 0, 4};
    static s16 cursor_steps[6][4] = {
        {-13, 13, -1, 1},
        {-15, 15, -1, 1},
        {-15, 15, -1, 1},
        {-19, 19, -1, 1},
        {-15, 15, -1, 1},
        {0, 0, 0, 0}
    };
    s32 keys;
    s32 event;
    s32 pushed;
    CDC2Mes *message;

    event = -1;
    keys = MenuCommonInfo->CheckSelectKey();
    keys = MenuCommonInfo->CheckLRKey();
    pushed = MenuCommonInfo->CheckPushButton();
    message = MenuDCMsg[7];

    switch (mode) {
    case NAMEREGI_MODE_OPEN:
        if (FadeCheckMenu() != 0) {
            mode = NAMEREGI_MODE_INPUT;
            MenuCommonInfo->key_enable = 1;
        }
        break;
    case NAMEREGI_MODE_CLOSE:
        if (FadeCheckMenu() != 0) {
            return 1;
        }
        break;
    case NAMEREGI_MODE_MESSAGE: {
        if (unk_6 == 0) {
            s32 answer = message->YesNoCursor2(1);
            if (answer == 1) {
                char converted_name[0x100];
                event = 0x1FE;
                MenuSePlay(1);
                if (Nameregi_Target.target == NAMEREGI_TARGET_KEYWORD) {
                    MenuArg.result[0] = 0;
                    event = 1;
                    strcpy(converted_name, name);
                    if (LanguageCode > 0) {
                        CopyJisToAscii(name, converted_name);
                    }
                    if (strcmp(Nameregi_Target.keyword, converted_name) == 0) {
                        MenuArg.result[0] = 1;
                    }
                    if (strcmp(Nameregi_Target.keyword, "SIRUS") == 0 && strcmp(converted_name, "Sirus") == 0) {
                        MenuArg.result[0] = 1;
                    }
                } else if (Nameregi_Target.target == NAMEREGI_TARGET_FISH) {
                    event = 0x3E8;
                    if (password_input != 0) {
                        event = 1;
                    }
                } else if (Nameregi_Target.target == NAMEREGI_TARGET_SPHIDA) {
                    event = 1;
                    strcpy(Nameregi_Target.keyword, name);
                    if (LanguageCode > 0) {
                        CopyJisToAscii(name, Nameregi_Target.keyword);
                    }
                }
            }
            if (answer == 2) {
                event = 0x1F9;
                MenuSePlay(5);
                if (Nameregi_Target.target == NAMEREGI_TARGET_KEYWORD) {
                    MenuArg.result[0] = -1;
                }
            }
        }
        if (unk_6 == 1 && pushed != 0) {
            event = 1;
            MenuSePlay(1);
        }
        s16 message_mode = unk_6;
        if (message_mode == 2) {
            if (pushed != 0) {
                event = 0x1F9;
            }
        }
        if (message_mode == 0xA) {
            s32 answer = message->YesNoCursor2(1);
            if (answer == 1) {
                event = 1;
                MenuSePlay(1);
                if (Nameregi_Target.target == NAMEREGI_TARGET_SPHIDA) {
                    Nameregi_Target.keyword[0] = 0;
                }
                if (Nameregi_Target.target == NAMEREGI_TARGET_FISH && Nameregi_Target.item != NULL) {
                    Nameregi_Target.item->item_no = -1;
                    Nameregi_Target.item->data.fish.name[0] = 0;
                }
            }
            if (answer == 2) {
                event = 0x1F9;
            }
        }
        if (unk_6 == 0x14) {
            s32 answer = message->YesNoCursor2(1);
            if (answer == 1) {
                event = 0x83;
            }
            if (answer == 2) {
                event = 0x1F9;
            }
        }
        message_mode = unk_6;
        if ((message_mode == 0x1E || message_mode == 0x28) && pushed != 0) {
            if (message_mode == 0x1E) {
                mode = NAMEREGI_MODE_INPUT;
            }
            if (unk_6 == 0x28) {
                mode = NAMEREGI_MODE_CLOSE;
                FadeOutMenu(0x28, 0.0f);
            }
            message_open = 0;
            MenuSePlay(5);
        }
        break;
    }
    case NAMEREGI_MODE_INPUT: {
        s32 font_mode = GetActiveFontMode();
        s16 *key_table = cursor_steps[font_mode];
        switch (key_arg_no) {
        case 0: {
            s8 japanese_navigation[12][4] = {
                {-1, 5, 11, 1},
                {-1, 7, 0, 2},
                {-1, 8, 1, 3},
                {-1, 10, 2, 4},
                {-1, 10, 3, 11},
                {0, -2, 11, 6},
                {0, -2, 5, 7},
                {1, -2, 6, 8},
                {1, -2, 7, 9},
                {2, -2, 8, 10},
                {3, -2, 9, 11},
                {-1, -2, 4, 0}
            };
            s8 localized_navigation[12][4] = {
                {-1, -1, -1, -1},
                {-1, -1, -1, -1},
                {-1, 5, 11, 3},
                {-1, 8, 2, 10},
                {-1, -1, -1, -1},
                {2, -2, 11, 6},
                {2, -2, 5, 8},
                {10, -2, 9, 11},
                {2, -2, 6, 9},
                {3, -2, 8, 7},
                {-1, 7, 3, 11},
                {-1, -2, 10, 2}
            };
            s8 *row = japanese_navigation[command_pos];
            if (LanguageCode > 0) {
                row = localized_navigation[command_pos];
            }
            s32 direction = -1;
            if (keys & 1) {
                direction = 0;
            }
            if (keys & 2) {
                direction = 1;
            }
            if (keys & 4) {
                direction = 2;
            }
            if (keys & 8) {
                direction = 3;
            }
            if (0 <= direction) {
                s8 target = row[direction];
                if (0 <= target) {
                    command_pos = target;
                    MenuSePlay(0);
                } else if (target == -2) {
                    ConvertPositionNameRegi(0);
                    key_arg_no = 1;
                    if (font_mode == NAMEREGI_FONT_MODE_KANJI) {
                        keys = 2;
                        CheckKanjiPosition(2, key_table, font_mode);
                    }
                    MenuSePlay(0);
                    break;
                }
            }
            s16 command_table[12][2] = {
                {20, 2},
                {20, 2},
                {20, 2},
                {20, 2},
                {20, 2},
                {70, 2},
                {71, 2},
                {100, 2},
                {110, 2},
                {120, 2},
                {130, 2},
                {500, 2}
            };
            s16 *command_events = command_table[command_pos];
            if ((pushed & 1) || (pushed & 4)) {
                event = command_events[0];
            } else if (pushed & 2) {
                event = command_events[1];
            }
            break;
        }
        case 1: {
            MENU_SELECT_PARAM *selection = &select;
            s32 result;
            s32 previous_position;
            if (font_mode == NAMEREGI_FONT_MODE_KANJI) {
                s32 previous_row = selection->row;
                if ((keys & 0x20) || (keys & 0x80)) {
                    selection->row += 6;
                }
                if ((keys & 0x10) || (keys & 0x40)) {
                    selection->row -= 6;
                }
                if (selection->row < 0) {
                    selection->row = 0;
                }
                s32 last_row = NameRegiMenuPtr->kanji_line_max;
                if (last_row < selection->row) {
                    selection->row = last_row;
                }
                if (previous_row != selection->row) {
                    MenuSePlay(0);
                }
            }
            previous_position = selection->pos;
            result = 0;
            if (font_mode == NAMEREGI_FONT_MODE_KANJI) {
                if (keys != 0) {
                    result = nameregist_local_key(selection, keys, key_table, font_mode);
                    if (0 <= result) {
                        result = CheckKanjiPosition(keys, key_table, font_mode);
                    }
                }
            } else {
                result = nameregist_local_key(selection, keys, key_table, font_mode);
            }
            if (result == -1) {
                ConvertPositionNameRegi(1);
                key_arg_no = 0;
                MenuSePlay(0);
            } else {
                if (previous_position != selection->pos) {
                    MenuSePlay(0);
                }
                if ((pushed & 1) || (pushed & 4)) {
                    event = 5;
                }
                if (pushed & 2) {
                    event = 0xB;
                }
            }
            break;
        }
        }
        break;
    }
    }

    switch (event) {
    case 0x14:
        if (password_input != 0) {
            MenuSePlay(5);
        } else {
            select_mode = command_pos;
            ChangeFontSelectMode(GetActiveFontMode());
            MenuSePlay(1);
        }
        break;
    case 0xA:
        key_arg_no = 1;
        select.row = 0;
        MenuSePlay(1);
        break;
    case 0xB:
        key_arg_no = 0;
        command_pos = character_buttons[GetActiveFontMode()];
        MenuSePlay(5);
        break;
    case 5: {
        char selected_character[0x20];
        GetSelectedActiveFont(selected_character);
        selected_character[2] = 0;
        for (s32 index = 0; index < name_pos; index++) {
            if (name[index] == 0) {
                name[index] = 0x20;
            }
        }
        name[name_pos] = selected_character[0];
        name_pos += 1;
        if (NameRegistMax <= name_pos) {
            name_pos = NameRegistMax - 1;
        }
        MenuSePlay(1);
        break;
    }
    case 0x46:
        name_pos -= 1;
        if (name_pos < 0) {
            name_pos = 0;
        }
        button_flash[5] = 8;
        caret_cnt = 0x28;
        MenuSePlay(1);
        break;
    case 0x47:
        name_pos += 1;
        if (NameRegistMax <= name_pos) {
            name_pos = NameRegistMax - 1;
        }
        button_flash[6] = 8;
        caret_cnt = 0x28;
        MenuSePlay(1);
        break;
    case 0x64: {
        MenuSePlay(5);
        s32 index = name_pos;
        if (index != 0) {
            for (; index < NameRegistMax; index++) {
                name[index - 1] = name[index];
            }
            name[NameRegistMax - 1] = 0;
            button_flash[7] = 8;
            name_pos -= 1;
            if (name_pos < 0) {
                name_pos = 0;
            }
            caret_cnt = 0x28;
        }
        break;
    }
    case 0x6E: {
        s32 index = name_pos;
        for (; index < NameRegistMax; index++) {
            name[index] = name[index + 1];
        }
        name[NameRegistMax] = 0;
        button_flash[8] = 8;
        caret_cnt = 0x28;
        MenuSePlay(5);
        break;
    }
    case 0x78: {
        s32 index = NameRegistMax;
        for (; name_pos <= index; index--) {
            name[index + 1] = name[index];
        }
        name[name_pos] = 0x20;
        name[NameRegistMax] = 0;
        button_flash[9] = 8;
        caret_cnt = 0x28;
        MenuSePlay(1);
        break;
    }
    case 0x82:
        if (Nameregi_Target.target == NAMEREGI_TARGET_FISH) {
            MenuSePlay(5);
        } else {

            mode = NAMEREGI_MODE_MESSAGE;
            message_open = 1;
            unk_6 = 0x14;
            message->MsgPreset(0xB);
            message->SetAbsPos(5);
            message->SetMsgCursor(0);
            message->MakeMsg(0x1007);
            char *arguments[2] = {NULL, NULL};
            if (Nameregi_Target.target == NAMEREGI_TARGET_ITEM) {
                arguments[0] = GetItemMessage(Nameregi_Target.item->item_no);
            }
            if (Nameregi_Target.target == NAMEREGI_TARGET_ROBO) {
                arguments[0] = GetUserDataMan()->GetRoboNameDefault();
            }
            if (Nameregi_Target.target == NAMEREGI_TARGET_KEYWORD) {
                message->MakeMsg(0x1008);
            }
            if (Nameregi_Target.target == NAMEREGI_TARGET_SPHIDA) {
                arguments[0] = NULL;
                message->MakeMsg(0x101B);
            }
            message->SetMsgItemNo(arguments, 2);
            MenuSePlay(1);
        }
        break;
    case 0x83:
        message_open = 0;
        mode = NAMEREGI_MODE_INPUT;
        MenuSePlay(1);
        memset(name, 0, sizeof(name));
        if (Nameregi_Target.target == NAMEREGI_TARGET_KEYWORD) {
            name_pos = 0;
        } else if (Nameregi_Target.target == NAMEREGI_TARGET_SPHIDA) {
            strcpy(name, Sfida_default_Name[LanguageCode]);
            name_pos = strlen(name);
        } else {
            strcpy(name, message->name[0]);
            if (LanguageCode > 0) {
                NameRegiMenuPtr->CopyAsciiToJis(message->name[0], name);
            }
            name_pos = strlen(name);
        }
        break;
    case 0x1FE: {
        char final_name[0x80];

        strcpy(final_name, name);
        if (Nameregi_Target.target == NAMEREGI_TARGET_ITEM) {
            if (LanguageCode > 0 && Nameregi_Target.item != NULL && Nameregi_Target.item->used_type == USED_ITEM_TYPE_WEAPON &&
                Nameregi_Target.item->IsFishingRod() == 0) {
                char ascii[0x80];
                s32 item_no;
                CopyJisToAscii(name, ascii);
                item_no = SearchItemByName(ascii);
                if (item_no == 0x12E || item_no == 0x12F) {
                    unk_6 = 0x1E;
                    message->MsgPreset(0xA);
                    message->SetAbsPos(5);
                    message->MakeMsg(0xFD4);
                    break;
                }
                if (ConvertUsedItemType(GetItemDataType(item_no)) == USED_ITEM_TYPE_WEAPON) {
                    Nameregi_Target.item->CopyDataWeapon(item_no);
                }
            }
            if (CheckDeleteNameRegisteItem(Nameregi_Target.item) != 0) {
                GetUserDataMan()->DeleteItem(0x180, 1);
            }
            if (LanguageCode > 0) {
                CopyJisToAscii(name, final_name);
            }
            Nameregi_Target.item->SetName(final_name);
            Nameregi_Target.item->rename_flag = 1;
        }
        if (Nameregi_Target.target == NAMEREGI_TARGET_ROBO) {
            GetUserDataMan()->SetRoboName(final_name);
        }
        if (Nameregi_Target.target == NAMEREGI_TARGET_SPHIDA) {
            if (LanguageCode > 0) {
                CopyJisToAscii(name, final_name);
            }
            strcpy(Nameregi_Target.keyword, final_name);
        }
        unk_6 = 1;
        message->MsgPreset(0xA);
        message->SetAbsPos(5);
        message->MakeMsg(0x1006);
        char *arguments[2] = {NULL, NULL};
        arguments[0] = old_name;
        arguments[1] = final_name;
        message->SetMsgItemNo(arguments, 2);
        break;
    }
    case 0x1F4: {
        s32 message_id;
        CheckInputWord(name);
        if ((s32)strlen(name) <= 0) {
            MenuSePlay(5);
            break;
        }
        message_id = 0x1005;
        if (Nameregi_Target.target == NAMEREGI_TARGET_FISH) {
            message_id = 0x1010;
            if (password_input != 0) {
                char password[0x30];
                u8 decoded[0x20];

                u16 header[7];
                u8 *key_text;
                s32 password_valid;
                ConvertShitJiss2Ascii(name, password);
                u8 key[0x21] = {0};
                password[0x16] = 0;
                strcpy((char *)key, Nameregi_Target.item->GetName(0));
                key_text = key;
                password_valid = DecodePassword(password, (u8 *)decoded, 0x10, (u8 *)key_text, 0x14);
                memcpy(header, decoded, 0xE);
                if (password_valid == 0 || (header[0] & 0x1FF) < 0x136) {
                    mode = NAMEREGI_MODE_MESSAGE;
                    message_open = 1;
                    unk_6 = 0x1E;
                    message->MsgPreset(0xA);
                    message->SetAbsPos(5);
                    message->ClsMes::mes_no = -1;
                    message->MakeMsg(0x1011);
                    message->SetMsgCursor(-1);
                    MenuSePlay(5);
                } else {
                    Nameregi_Target.item->Init();
                    Nameregi_Target.item->used_type = USED_ITEM_TYPE_FISH;
                    Nameregi_Target.item->SetName((char *)key_text);
                    Nameregi_Target.item->TransToData((char *)decoded, 0xE);
                    MenuSePlay(1);
                    mode = NAMEREGI_MODE_MESSAGE;
                    message->MsgPreset(0xA);
                    message->SetAbsPos(5);
                    message->ClsMes::mes_no = -1;
                    unk_6 = 0x28;
                    message_open = 1;
                    Nameregi_Target.item->TransToData((char *)decoded, 0xE);
                    message->MakeMsg(0x1012);
                    if (key_text != NULL) {
                        strcpy(message->name[0], (char *)key_text);
                    }
                }
                break;
            }
        } else if (Nameregi_Target.target == NAMEREGI_TARGET_SPHIDA) {
            message_id = 0x101A;
        } else if (Nameregi_Target.target != NAMEREGI_TARGET_KEYWORD) {
            char candidate_name[0x80];
            strcpy(candidate_name, name);
            if (LanguageCode > 0) {
                CopyJisToAscii(name, candidate_name);
            }
            if (strcmp(candidate_name, old_name) == 0) {
                mode = NAMEREGI_MODE_MESSAGE;
                message_open = 1;
                unk_6 = 0xA;
                message->MsgPreset(0xB);
                message->SetAbsPos(5);
                message->SetMsgCursor(0);
                message->MakeMsg(0xFDC);
                char *arguments[2] = {NULL, NULL};
                arguments[0] = old_name;
                message->SetMsgItemNo(arguments, 2);
                MenuSePlay(1);
                break;
            }
        } else {
            message_id = 0x1004;
            MenuArg.end_code = 0xE;
        }
        {
            char display_name[0x80];
            mode = NAMEREGI_MODE_MESSAGE;
            unk_6 = 0;
            message_open = 1;
            message->MsgPreset(0xB);
            message->SetAbsPos(5);
            message->SetMsgCursor(1);
            message->MakeMsg(message_id);
            char *arguments[2] = {NULL, NULL};
            arguments[0] = name;
            if (LanguageCode > 0) {
                CopyJisToAscii(name, display_name);
                arguments[0] = display_name;
            }
            message->SetMsgItemNo(arguments, 1);
            MenuSePlay(1);
        }
        break;
    }
    case 0x1F9:
        mode = NAMEREGI_MODE_INPUT;
        message_open = 0;
        MenuSePlay(5);
        break;
    case 2:
        if (Nameregi_Target.target == NAMEREGI_TARGET_KEYWORD) {
            unk_6 = 0xA;
            message_open = 1;
            MenuArg.end_code = 0;
            message->MsgPreset(0xB);
            message->SetAbsPos(5);
            message->SetMsgCursor(1);
            mode = NAMEREGI_MODE_MESSAGE;
            message->MakeMsg(0xFB4);
            MenuSePlay(5);
        } else {

            mode = NAMEREGI_MODE_MESSAGE;
            unk_6 = 0xA;
            message_open = 1;
            message->MsgPreset(0xB);
            message->SetAbsPos(5);
            message->SetMsgCursor(1);
            message->MakeMsg(0xFDC);
            char *arguments[2] = {NULL, NULL};
            arguments[0] = old_name;
            message->SetMsgItemNo(arguments, 1);
            MenuSePlay(5);
            if (Nameregi_Target.target == NAMEREGI_TARGET_SPHIDA) {
                message->MsgPreset(0xB);
                message->SetAbsPos(5);
                message->MakeMsg(0x1019);
                message->SetMsgCursor(1);
            } else if (Nameregi_Target.target == NAMEREGI_TARGET_FISH) {
                message->MsgPreset(0xB);
                message->SetAbsPos(5);
                message->MakeMsg(0x1013);
                message->SetMsgCursor(1);
            }
        }
        break;
    case 0x3E8: {
        if (Nameregi_Target.target == NAMEREGI_TARGET_FISH) {
            Nameregi_Target.item->item_no = 0x140;
            Nameregi_Target.item->used_type = USED_ITEM_TYPE_FISH;
            if (LanguageCode > 0) {
                char backup[0x40];
                strcpy(backup, name);
                CopyJisToAscii(backup, name);
            }
            Nameregi_Target.item->SetName(name);
        }
        MenuDCMsg[6]->MakeMsg(0x100F);
        CDC2Mes *name_window = MenuDCMsg[6];
        char *name_text = name;
        if (name_text != NULL) {
            strcpy(name_window->name[0], name_text);
        }
        AdjustWaku(MenuDCMsg[6], &NameRegiMenuPtr->waku);
        memset(name, 0, sizeof(name));
        name_pos = 0;
        password_input = 1;
        command_pos = 2;
        select_mode = command_pos;
        ChangeFontSelectMode(GetActiveFontMode());
        NameRegistMax = 0x16;
        mode = NAMEREGI_MODE_INPUT;
        step = 0;
        message_open = 0;
        break;
    }
    case 1:
        mode = NAMEREGI_MODE_CLOSE;
        FadeOutMenu(0x28, 0.0f);
        break;
    }

    caret_cnt += 1;
    if (caret_cnt >= 0x50) {
        caret_cnt = 0;
    }
    wave_angle += 0.06981317f;
    if (wave_angle > 3.1415927f) {
        wave_angle -= 6.2831855f;
    }
    StepMarkCursor();
    MenuDCMsg[6]->StepMsg();
    message->StepMsg();
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/nameregi", GetSelectedActiveFont__13CNameRegiMenuFPc);
void CNameRegiMenu::ChangeFontSelectMode(int font_mode) {
    if (font_mode < 0 || font_mode >= NAMEREGI_FONT_MODE_NUM) {
        return;
    }
    int spacing_x = 0x18;
    int spacing_y = spacing_x;
    if (font_mode == NAMEREGI_FONT_MODE_KANJI) {
        spacing_x = 0x16;
    }
    if (font_mode == NAMEREGI_FONT_MODE_ALPHA) {
        spacing_x = 0x30;
        spacing_y = 0x18;
    }
    if (font_mode == NAMEREGI_FONT_MODE_KIGOU) {
        spacing_x = 0x30;
        spacing_y = 0x18;
    }
    grid_font[0].Init();
    grid_font[0].SetFuchi(5);
    grid_font[0].SetColor(0x80686A6BU);
    grid_font[0].SetClearance(spacing_x, spacing_y);
    grid_font[0].unk_b0 = 0.0f;
    grid_font[0].unk_b4 = 0.0f;
}

s8 ConvertNameRegiBaseBoardTable(int font_mode) {
    static s8 buttons[5] = {2, 1, 0, 4, 3};
    s8 result = buttons[font_mode];
    if (LanguageCode > 0) {
        s8 alternate[5] = {0, 0, 0, 0, 4};
        result = alternate[font_mode];
    }
    return result;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/nameregi", DrawBaseBoard__13CNameRegiMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/nameregi", DrawActiveFont__13CNameRegiMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/nameregi", StepMarkCursor__13CNameRegiMenuFv);
void CNameRegiMenu::DrawMarkCursor() {
    float pos[2];
    pos[0] = cursor_x + 6.0f * cosf(mgAngleLimit(0.05235988f * (float)cursor_cnt));
    pos[1] = cursor_y + 4.0f * sinf(mgAngleLimit(0.10471976f * (float)cursor_cnt));
    MenuCursorDraw(NameRegiCursor, pos, 0.0f, 0, 0x80, 0.8f);
}

void CNameRegiMenu::DrawSelectedWord() {
    static s16 frame_texture[12] = {0, 148, 42, 64, 42, 148, 16, 64, 58, 148, 42, 64};
    mgRect<int> shadow;
    mgRect<int> frame;
    MenuReloadTexture(OldReloadTexNumber, NameRegiTex1->block);
    int box_width = NameRegistMax * 0xC + 0x3E;
    int box_left = (mgScreenWidth - box_width) >> 1;
    mgCDrawPrim *prim = GetMenuPrim();
    SetSpriteEnv(prim, 0);
    prim->Begin(MG_PRIM_SPRITE);
    prim->Texture(NameRegiTex1);
    prim->Color(0, 0, 0, 0x33);
    shadow.Set(box_left + 3, 0x59, box_width, frame_texture[3]);
    Menu3DivideTextureDraw(prim, shadow, frame_texture, 1);
    prim->Color(0x80, 0x80, 0x80, 0x80);
    frame.Set(box_left, 0x56, box_width, frame_texture[3]);
    Menu3DivideTextureDraw(prim, frame, frame_texture, 1);
    prim->End();
    int underscore_x = box_left + 0x20;
    SetSpriteEnv(prim, 2);
    prim->Begin(MG_PRIM_SPRITE);
    prim->Color(0xFA, 0xFA, 0xFA, 0x40);
    int i = 0;
    while (i < NameRegistMax) {
        prim->Vertex(underscore_x, 0x7F, 0);
        prim->Vertex(underscore_x + 8, 0x81, 0);
        underscore_x += 0xC;
        i++;
    }
    prim->End();
    int cursor_alpha = 0x60;
    if (caret_cnt % 80 < 0x28) {
        cursor_alpha = 0;
    }
    int cursor_left = box_left + 0x1E + name_pos * 0xC;
    prim->Begin(MG_PRIM_SPRITE);
    prim->Color(0xDC, 0xDC, 0xDC, cursor_alpha);
    prim->Vertex(cursor_left, 0x65, 0);
    prim->Vertex(cursor_left + 0xC, 0x7C, 0);
    prim->End();
    MenuReloadTexture(OldReloadTexNumber, MenuArg.mes_tex_block);
    name_font.SetPos(box_left + 0x1F, 0x67);
    name_font.SetStr(name);
    CFont *font = &name_font;
    font->DrawDirect(font->str, font->pos_x, font->pos_y);
}

void CNameRegiMenu::DrawMessage() {

    MenuReloadTexture(OldReloadTexNumber, MenuDCMsg[6]->texture_block);
    mgCDrawPrim prim;
    SetSpriteEnv(&prim, 0);
    RGBAQ_TYPE color = {128, 128, 128, 128, 1.0f};
    DrawVersatileWin_1(&prim, waku, &color, 0x80);
    (MenuDCMsg[6])->DrawMsg();
    if (message_open != 0) {
        DrawMenuFillBox(0.0f, 0.0f, (float)mgScreenWidth, (float)mgScreenHeight, 0x40, 0, 0,
                       0);
        (MenuDCMsg[7])->DrawMsg();
    }
}

// Static initialiser (.init)
INCLUDE_ASM("ps2/asm/pal/nonmatchings/nameregi", __sinit_nameregi_cpp);

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", Sfida_default_Name__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", ALPHA_TABLE1__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", ALPHA_TABLE2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", STR_NUM_TABLE__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", KIGOU_TABLE_ASCII1__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", KIGOU_TABLE_ASCII2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", ascii_code_table__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", NameRegistFont_Table__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", NameStrSelectModeTable__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", NameRegiSearchKanjiIndexTable__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", testchar__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", txt_table__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", txt_table2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1153__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", LimmitTable_1360__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1377__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", Convtable2_1382__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", addTable_1510__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1513__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1514__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1534__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", nameregist_baseboard_upper_table__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", colt_1808__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", table_1819__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", tex_commtbl_1822__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", gettbl0_2012__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_892__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_893__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1281__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1282__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1283__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1284__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1285__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1286__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1287__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1288__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1747__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1748__2__DATA);

// Static initialiser table (.ctor)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", D_0037B084__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", __vt__13CNameRegiMenu__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", NameRegistMax__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", HIRA_TABLE1__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", HIRA_TABLE2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", HIRA_TABLE3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", KATA_TABLE1__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", KATA_TABLE2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", KATA_TABLE3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", KIGOU_TABLE1__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", KIGOU_TABLE2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", jis_ptr_table__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", NameRegistGyouLimmitTable__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1081__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", convTbl_1579__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", convtbl_1792__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1795__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", get_Htable_1806__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_1807__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/nameregi", at_2031__3__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(NameRegiCode, 0x4);
INCLUDE_BSS(NameRegiMenuPtr, 0x4);
INCLUDE_BSS(OldReloadTexNumber, 0x4);
INCLUDE_BSS(NameRegiTex1, 0x4);
INCLUDE_BSS(NameRegiBGTile, 0x4);
INCLUDE_BSS(NameRegiCursor, 0x4);
INCLUDE_BSS(NameRegiWaku, 0x4);
INCLUDE_BSS(NameregiGaiji, 0x8);
INCLUDE_BSS(at_1621__3, 0x8);
INCLUDE_BSS(at_1661__3, 0x8);
INCLUDE_BSS(at_1684__3, 0x8);
INCLUDE_BSS(at_1686, 0x8);
INCLUDE_BSS(at_1693__2, 0x8);

// Uninitialised data (.bss)
INCLUDE_BSS(Nameregi_Target, 0x50);
INCLUDE_BSS(NameRegiTopic, 0x40);
INCLUDE_BSS(NameRegiStack, 0x30);
INCLUDE_BSS(at_1171__3, 0x10);
INCLUDE_BSS(at_1669, 0x28);
INCLUDE_BSS(at_1755, 0x18);
