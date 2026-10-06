#include "common.h"
#include "menushop.hpp"
#include "inventmn.hpp"
#include "quest.hpp"
#include "sound.hpp"
#include "dataread.hpp"
#include "prespr.hpp"
#include "mg_drawprim.hpp"
#include <cstdio>
#include <cstring>
#include "menusystemdata.hpp"
#include "font.hpp"
#include "sysmes.hpp"
#include "scenesnd.hpp"
#include "savedata.hpp"
#include "userdata.hpp"
#include "gamedata.hpp"
#include "scriptinterpreter.hpp"
#include "mg_math.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "mainloop.hpp"
#include "menucls1.hpp"
#include "menucommon.hpp"
#include "menudraw.hpp"
#include "menusys.hpp"
#include "menumain.hpp"

static short NowSellMode;
static SHOP_PRICE_INFO * Spi_PriceList;
static DONY_SHOP_ITEM dony_shoplist[8] = {
    {189, 1},
    {115, 2},
    {205, 3},
    {122, 4},
    {261, 5},
    {428, 6},
    {427, 7},
    {-1, 1}
};
static short Now_Shop_ID;
static int * Now_ShopDataReadPtr;
static short Now_ShopListNum;

static CShopMenu *CShopMenuPt;
static CMenuQuestView *MenuQuestView;

static int CheckRobotCore(void);
static int _SHOP_ANALYZE(SPI_STACK *stack, int argc);
static int _PRICE(SPI_STACK *stack, int argc);

static SPI_TAG_PARAM menu_shop_tag[] = {
    {"SHOP", _SHOP_ANALYZE},
    {"PRICE", _PRICE},
    {NULL, NULL}
};

// Code (.text)
int GetDonyShopLineUp(int *item_no, int *status) {
    CInventUserData *invent_data = GetInventUserDataPtr();
    CMenuSystemData *system_data = GetMenuSysData();
    if (system_data == NULL || invent_data == NULL) {
        return 0;
    }
    int level = invent_data->GetLevel();
    int already_owned = 0;
    int listed = 0;
    int available = 0;
    DONY_SHOP_ITEM *entry = dony_shoplist;
    for (; 0 < entry->item_no; entry++) {
        listed++;
        if (system_data->CheckGetAlready(entry->item_no) != 0) {
            already_owned++;
        } else if (entry->level < level) {
            if (item_no != NULL) {
                item_no[available] = entry->item_no;
            }
            available++;
        }
    }
    if (status != NULL) {
        if (listed == already_owned) {
            *status = DONY_SHOP_STATUS_ALL_TAKEN;
        } else {
            if (available <= 0) {
                *status = DONY_SHOP_STATUS_NONE_YET;
            } else if (available == already_owned) {
                *status = DONY_SHOP_STATUS_OFFERED;
            }
        }
    }
    return available;
}

void CShop::CheckSyojiHin() {
    CUserDataManager *user_data = GetUserDataMan();
    for (int i = 0; i < item_num; i++) {
        have_num[i] = 0;
        if (item_no[i] > 0) {
            have_num[i] = user_data->GetNumSameItem(item_no[i]);
        }
    }
}

/**
 * Checks which robot core the player can use.
 */
static int CheckRobotCore(void) {
    return (GetUserDataMan())->CheckRobotCore();
}

void CShop::CheckEventItem() {
    CUserDataManager *user_data = &GetSaveData()->user_data;
    if (NowSellMode == SHOP_SELL_MODE_DONY) {
        item_num = GetDonyShopLineUp(item_no, NULL);
        return;
    }
    int cursor = 0;
    while (cursor < item_num) {
        if (item_no[cursor] == 0x173 && CheckBitFlagMenu(0x1B) != 0) {
            local_sort1(cursor, &item_num, item_no);
        }
        if (item_no[cursor] == 0xAC && user_data->GetNumSameItem(0xAC) > 0) {
            local_sort1(cursor, &item_num, item_no);
        }
        if (item_no[cursor] == 0x1A6 && user_data->CheckVoiceUnit() != 0) {
            local_sort1(cursor, &item_num, item_no);
            cursor -= 1;
        }
        if (item_no[cursor] == 0x1A7 && (once_item_chosen == 1 || user_data->unk_44dc0 >= 0x15)) {
            local_sort1(cursor, &item_num, item_no);
        }
        if (item_no[cursor] == 0x163 && user_data->GetNumSameItem(0x163) > 0) {
            local_sort1(cursor, &item_num, item_no);
        }
        if (item_no[cursor] == 0x12F && user_data->GetNumSameItem(0x12F) > 0) {
            local_sort1(cursor, &item_num, item_no);
        }
        if (item_no[cursor] == 0x166 && user_data->GetNumSameItem(0x166) > 0) {
            local_sort1(cursor, &item_num, item_no);
        }
        if (GetItemDataType(item_no[cursor]) == 0xB) {
            int core = CheckRobotCore();
            if (core >= 0xF6 && core < 0xFC) {
                item_no[cursor] = core + 1;
            } else {
                local_sort1(cursor, &item_num, item_no);
                cursor -= 1;
            }
        }
        if (item_no[cursor] == 0x1A8 &&
            user_data->GetMonsterBajjiDataPtr(4)->enable != 0) {
            local_sort1(cursor, &item_num, item_no);
        }
        if (GetQuestRequestStatus(2) == 2 &&
            (item_no[cursor] == 0xC9 || item_no[cursor] == 0xCA)) {
            local_sort1(cursor, &item_num, item_no);
            cursor -= 1;
        }
        cursor += 1;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", GetPrice__5CShopFP13CGameDataUsedPiPi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", CheckMoney__5CShopFv);
int CShop::AddMoney(int amount) {
    if (NowSellMode == SHOP_SELL_MODE_MONEY) {
        return GetUserDataMan()->AddMoney(amount);
    }
    if (NowSellMode == SHOP_SELL_MODE_ROBO_ABS) {
        return (int)GetUserDataMan()->AddRoboAbs((float)amount);
    }
    if (NowSellMode == SHOP_SELL_MODE_MEDAL) {
        return GetUserDataMan()->AddYarikomiMedal(amount);
    }
    if (NowSellMode == SHOP_SELL_MODE_DONY) {
        return 0;
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", _SHOP_ANALYZE__FP9SPI_STACKi);
/**
 * Stores the buying and selling prices of an item.
 */
static int _PRICE(SPI_STACK *stack, int argc) {
    int item_id = spiGetStackInt(stack++);
    Spi_PriceList[item_id].buy = spiGetStackInt(stack++);
    Spi_PriceList[item_id].sell = spiGetStackInt(stack++);
    return 1;
}

void CShop::AnalyzeShopList(char *script, int length) {
    Now_Shop_ID = shop_id;
    Now_ShopDataReadPtr = item_no;
    Spi_PriceList = price;
    NowSellMode = 0;
    CScriptInterpreter interpreter;
    interpreter.SetTag(menu_shop_tag);
    interpreter.SetScript(script, length);
    interpreter.Run();
    item_num = Now_ShopListNum;
    CheckEventItem();
    CheckSyojiHin();
}

void CShopMenu::AttachForm() {
    money_brd = MenuPosData->GetFormInfo("moneybrd");
    trade_brd = MenuPosData->GetFormInfo("\224\204\224\203\203{\201[\203h");
    shop_name_brd = MenuPosData->GetFormInfo("\223X\226\274\203{\201[\203h");
    item_brd = MenuPosData->GetFormInfo("\225i\225\250\203{\201[\203h");
    item_list = MenuPosData->GetFormInfo("\225i\225\250\203\212\203X\203g");
    exp_brd = MenuPosData->GetFormInfo("EXPBRD");
    medal_brd = MenuPosData->GetFormInfo("MEDALBRD");
    if (item_list != NULL) {
        item_list->GetPutPosXY(NULL, list_x, list_y);
        list_y += 48.0f;
    }
    GiftBoxViewForm = MenuPosData->GetFormInfo("giftview");
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", IsCancelNoneLoadItem__9CShopMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", UpdataScrlBar__9CShopMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", InitEnd__9CShopMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", KeyStep__9CShopMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", CalcTex__9CShopMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", CalcCursorPosition__9CShopMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", SearchNowPosItemExist__9CShopMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", ShopSellListDraw__FRiPf);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", MenuShopInit__FP9mgCMemoryPii);
int MenuShopKey() {
    return CShopMenuPt->KeyStep();
}

void MenuShopDraw() {
    MenuPosData->FormDraw();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", UnderMsg__14CMenuQuestViewFi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", SelectMax__14CMenuQuestViewFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", InitEnd__14CMenuQuestViewFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", KeyStep__14CMenuQuestViewFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", MenuNPCQuestViewInit__FP9mgCMemoryPii);
int MenuNPCQuestViewKey() {
    return MenuQuestView->KeyStep();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", MenuNPCQuestViewDraw__Fv);

// Static initialiser (.init)
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menushop", __sinit_menushop_cpp);

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", imglist_1267__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", extbl_1278__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", exe_tbl_1509__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", extbl_1573__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", extbl_1589__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", randam_checktbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", tbl_2469__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2470__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1221__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1222__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1223__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1224__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1225__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1226__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1227__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1228__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1252__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1253__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1254__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1268__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1269__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1270__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1279__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1280__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1281__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1282__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1298__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1299__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1300__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1301__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1302__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1303__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1304__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1305__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1306__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1307__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1308__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1510__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1511__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1512__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1513__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1574__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1575__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1576__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1590__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1591__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1592__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1650__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1651__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1652__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1653__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1654__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1655__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1656__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1657__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1658__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1659__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1660__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1661__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1662__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1663__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1664__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1665__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1666__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1667__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1817__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1818__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1819__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1820__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1821__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1822__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1823__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1824__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1825__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1826__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1839__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1840__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_1881__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2114__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2115__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2116__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2117__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2118__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2172__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2173__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2219__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2220__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2221__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2222__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", at_2629__2__DATA);

// Static initialiser table (.ctor)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", D_0037B048__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", __vt__14CMenuQuestView__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", __vt__9CShopMenu__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", t_offxy_1832__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", cursor_offsetxy_1836__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", cursortbl_1838__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", rgba_1897__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", QuestMoveRate__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menushop", packname_2171__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(NowSellMode, 0x4);
INCLUDE_BSS(CShopPtr, 0x4);
INCLUDE_BSS(Tex_Shop, 0x4);
INCLUDE_BSS(Tex_Mt0, 0x4);
INCLUDE_BSS(Now_ShopListNum, 0x4);
INCLUDE_BSS(Now_ShopDataReadPtr, 0x4);
INCLUDE_BSS(Now_Shop_ID, 0x4);
INCLUDE_BSS(Spi_PriceList, 0x4);
INCLUDE_BSS(shop_mode_prev_1326, 0x4);
INCLUDE_BSS(init_1327, 0x8);
INCLUDE_BSS(at_1581__2, 0x8);
INCLUDE_BSS(at_1582__3, 0x8);
INCLUDE_BSS(at_1595__3, 0x8);
INCLUDE_BSS(at_1685__2, 0x8);
INCLUDE_BSS(at_1831__2, 0x8);
INCLUDE_BSS(CShopMenuPt, 0x4);
INCLUDE_BSS(QuestMan, 0x4);
INCLUDE_BSS(QuestDataPtr, 0x4);
INCLUDE_BSS(Tex_QuestMemo, 0x4);
INCLUDE_BSS(QuestMenuMes, 0x4);
INCLUDE_BSS(ActiveQuestInfo, 0x4);
INCLUDE_BSS(QuestTilePatternXY, 0x8);
INCLUDE_BSS(QuestCursorPos, 0x8);
INCLUDE_BSS(QuestListTopY, 0x4);
INCLUDE_BSS(QuestCommentWinX, 0x4);
INCLUDE_BSS(QuestScrlBarY, 0x4);
INCLUDE_BSS(QuestScrlBarH, 0x4);
INCLUDE_BSS(QuestViewCommentFlag, 0x4);
INCLUDE_BSS(QuestReactionCommentGyouNum, 0x4);
INCLUDE_BSS(ScoopMan, 0x4);
INCLUDE_BSS(ScmFlagCtrl, 0x4);
INCLUDE_BSS(menu_debug_questselect, 0x4);
INCLUDE_BSS(Menu_Memo_ViewMode, 0x4);
INCLUDE_BSS(MenuQuestView, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(MenuLocalStack, 0x30);
INCLUDE_BSS(QuestCommentMes, 0x10);
