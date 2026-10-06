#include "common.h"
#include "movieviewlp.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "gaiji.hpp"
#include "dataread.hpp"
#include "movie.hpp"
#include "scenesnd.hpp"
#include "gamepad.hpp"
#include "prespr.hpp"
#include "font.hpp"
#include <cstdio>
#include <cstring>
#include "mglib.hpp"
#include "scriptinterpreter.hpp"
#include "snd_mngr.hpp"

static CScene *MovieScene;
static CMovie *MovieView;
static mgCTexture *RushWork;
static MOVIE_LIST_ENTRY *MovieList;
static int MovieListNum;
static short MovieLine;
static short MovieSelect;
static short MovieSpecialMode;
static short MovieSpecialModeInfo[3];
static int MovieMode;
static mgCMemory *spi_MovieStack;
static int performance_meter_flag;
static mgCMemory DataBuffer;
static mgCMemory Stack_ReadBuff;

static int _MOVIE(SPI_STACK *args, int argc);
static SPI_TAG_PARAM tag_movie[] = {
    { "MOVIE", _MOVIE },
    { NULL, NULL }
};

/**
 * Returns the free portion of a memory stack in quadwords.
 */
static inline int movieFreeBlocks(mgCMemory *memory) {
    return memory->stack_size - memory->stack_used;
}

/**
 * Returns the next free quadword of a memory stack.
 */
static inline u_long128 *movieFreeTop(mgCMemory *memory) {
    return &memory->stack[memory->stack_used];
}

// Code (.text)
/**
 * Adds a movie and its optional background music to the viewer's list.
 */
static int _MOVIE(SPI_STACK *args, int argc) {
    MOVIE_LIST_ENTRY *entry = &MovieList[MovieListNum];
    if (entry == NULL) {
        return 0;
    }
    char *title = spiGetStackString(args++);
    char *file_name = spiGetStackString(args++);
    int bgm_no = -1;
    if (argc >= 3) {
        bgm_no = spiGetStackInt(args);
    }
    if (entry != NULL) {
        entry->name = mgCopyString(title, spi_MovieStack);
        entry->file_name = mgCopyString(file_name, spi_MovieStack);
        entry->bgm_no = bgm_no;
    }
    MovieListNum++;
    return 1;
}

void MovieViewInit(INIT_LOOP_ARG arg) {
    mgCMemory *main_stack;
    mgCTextureManager *textures;
    u_long128 *vif0;
    u_long128 *vif1;
    char script[0x5000];
    int script_size;
    int read_size;
    char *script_ptr;

    MovieScene = GetMainScene();
    MovieScene->Initialize();
    mgInitFont();
    main_stack = GetMainStack();
    main_stack->stack_used = 0;
    main_stack->lock = 0;
    static mgCMemory buf0;
    static mgCMemory buf1;
    static mgCMemory dbuf0;
    static mgCMemory dbuf1;
    vif0 = main_stack->stAlloc64(10000);
    vif1 = main_stack->stAlloc64(10000);
    mgInitVif1Packet(vif0, vif1, 0x27100);
    buf0.stSetBuffer(main_stack->stAlloc64(30000), 30000);
    buf1.stSetBuffer(main_stack->stAlloc64(30000), 30000);
    dbuf0.stSetBuffer(main_stack->stAlloc64(60000), 60000);
    dbuf1.stSetBuffer(main_stack->stAlloc64(60000), 60000);
    DataBuffer.stSetBuffer(main_stack->stAlloc64(100000), 100000);
    mgSetPacketBuffer(&buf0, &buf1);
    mgSetDataBuffer(&dbuf0, &dbuf1, 1);
    mgSetBackGround(0.0f, 0.0f, 0.0f, 128.0f);
    SetTextureTable(100, 20, &DataBuffer);
    textures = &mgTexManager;
    mgTexManager.EnterIMGFile(GetGaijiImgPtr(), 0, NULL, NULL);
    ReLoadFontTexture(0);
    mgTexManager.EnterIMGFile(GetFontTex2ImgPtr(), 0, NULL, NULL);
    MovieView = new ((u_long128 *)main_stack->Alloc(0x2396)) CMovie;
    MovieListNum = 0;
    MovieList = new ((u_long128 *)main_stack->Alloc(0x32)) MOVIE_LIST_ENTRY[64];
    MovieLine = 0;
    MovieSelect = 0;
    MovieMode = MOVIE_VIEW_MODE_SELECT;
    spi_MovieStack = main_stack;
    script_ptr = script;
    if (LoadFile2("mv.cfg", script_ptr, &script_size, 0) != 0) {
        CScriptInterpreter interpreter;
        interpreter.SetTag(tag_movie);
        interpreter.SetScript(script_ptr, script_size);
        interpreter.Run();
    }
    main_stack->Align64();
    read_size = movieFreeBlocks(main_stack);
    Stack_ReadBuff.stSetBuffer(movieFreeTop(main_stack), read_size);
    Stack_ReadBuff.stack_used = 0;
    Stack_ReadBuff.lock = 0;
    Stack_ReadBuff.Align64();
    MovieSpecialModeInfo[0] = 0;
    MovieSpecialModeInfo[1] = 0;
    MovieSpecialMode = MOVIE_SPECIAL_MODE_NONE;
    MovieSpecialModeInfo[2] = 0;
    textures->EnterTexture(10, "moviework", NULL, mgScreenWidth, mgScreenHeight, mgScreenDepth,
                           0, 0, 0);
    RushWork = textures->GetTexture("moviework", 10);
    performance_meter_flag = mgGetPerformanceMeterFlag();
    mgPerformanceMeter(0);
}

void MovieViewExit() {
    sndSeAllStop(-1);
    mgCloseFont();
    mgPerformanceMeter(performance_meter_flag);
}

int MovieViewLoop(void) {
    mgCTextureManager *textures = &mgTexManager;
    MOVIE_LIST_ENTRY *entry;
    int y;
    int row;

    if (MovieMode == MOVIE_VIEW_MODE_SELECT) {
        if (GamePad__2.Down(PAD_START) || GamePad__2.Down(PAD_CROSS)) {
            return 1;
        }
        if (GamePad__2.Down(PAD_UP) != 0) {
            MovieSelect -= 1;
        }
        if (GamePad__2.Down(PAD_DOWN) != 0) {
            MovieSelect += 1;
        }
        if (GamePad__2.Down(PAD_L1) != 0) {
            MovieSelect -= 7;
        }
        if (GamePad__2.Down(PAD_R1) != 0) {
            MovieSelect += 7;
        }
        if (MovieSelect < 0) {
            MovieSelect = 0;
        }
        if (MovieListNum <= MovieSelect) {
            MovieSelect = MovieListNum - 1;
        }
        if (MovieSelect < MovieLine) {
            MovieLine -= 1;
        }
        if (MovieLine < 0) {
            MovieLine = 0;
        }
        if (MovieLine + 7 < MovieSelect) {
            MovieLine += 1;
        }
        if (GamePad__2.Down(PAD_CIRCLE) != 0) {
            Stack_ReadBuff.stack_used = 0;
            Stack_ReadBuff.lock = 0;
            entry = &MovieList[MovieSelect];
            textures->ReloadTexture(10, (sceVif1Packet *)NULL);
            if (0 < entry->bgm_no) {
                MovieScene->StopBGM(0);
                MovieScene->LoadBGM(
                    entry->bgm_no,
                    &Stack_ReadBuff.stack[Stack_ReadBuff.stack_used]);
                MovieScene->PlayBGM(0, -1, 1.0f);
            }
            MovieSpecialMode = MOVIE_SPECIAL_MODE_NONE;
            if (strcmp(entry->name, "promo") == 0) {
                MovieSpecialMode = MOVIE_SPECIAL_MODE_PROMO;
                MovieSpecialModeInfo[0] = 1;
                MovieView->Load("PROMO1.PSS", &Stack_ReadBuff, 0x200, 0x1A0, true, false);
                MovieView->Play("moviework");
                MovieView->SwitchThread();
                while (MovieView->IsStarted() == 0) {
                    MovieView->SwitchThread();
                }
            } else if (strcmp(entry->name, "promo_tv") == 0) {
                MovieSpecialMode = MOVIE_SPECIAL_MODE_PROMO_TV;
                MovieSpecialModeInfo[0] = 1;
                MovieView->Load("PROMO1TV.PSS", &Stack_ReadBuff, 0x200, 0x1A0, true, false);
                MovieView->Play("moviework");
                MovieView->SwitchThread();
                while (MovieView->IsStarted() == 0) {
                    MovieView->SwitchThread();
                }
            } else {
                MovieView->Load(entry->file_name, &Stack_ReadBuff, 0x200, 0x1A0, true, false);
                MovieView->Play("moviework");
                MovieView->SwitchThread();
                while (MovieView->IsStarted() == 0) {
                    MovieView->SwitchThread();
                }
            }
            MovieMode = MOVIE_VIEW_MODE_PLAY;
        }
        textures->ReloadTexture(0, (sceVif1Packet *)NULL);
        CFont font;
        char label[0x100];
        font.Init();
        font.SetClearance(16, 20);
        font.SetFuchi(5);
        font.SetColor(0x80686A6BU);
        sprintf(label, "  :%18s     %s", "\x89\x66\x91\x9C\x20\x20", "BGMID");
        row = MovieLine;
        y = 0x28;
        while (row < MovieLine + 8 && row < MovieListNum) {
            sprintf(label, "  %d:%18s  ", row, MovieList[row].name);
            if (row == MovieSelect) {
                label[1] = '>';
            }
            font.SetStr(label);
            font.SetPos(40, y);
            font.DrawDirect(font.str, font.pos_x, font.pos_y);
            y += 0x14;
            if (y >= 0xC9) {
                break;
            }
            row++;
        }
        return 0;
    }
    if (MovieMode == MOVIE_VIEW_MODE_PLAY) {
        mgPerformanceMeter(0);
        mgTexManager.ReloadTexture(10, (sceVif1Packet *)NULL);
        MovieView->SwitchThread();
        CPreSprite sprite;
        sprite.Initialize(NULL, NULL);
        sprite.Preset2D();
        sprite.AlphaBlendEnable(0);
        sprite.TextureMapEnable(1);
        sprite.Begin(MG_PRIM_SPRITE);
        sprite.Color(0, 0, 0, 0x80);
        sprite.SetIRect(0, 0, 0x200, 0x1A0, 0, 0);
        sprite.Texture(RushWork);
        sprite.Color(0x80, 0x80, 0x80, 0x80);
        sprite.SetIRect(0, 0, 0x200, mgScreenHeight, 0, 0);
        sprite.End();
        if (GamePad__2.Down(PAD_R1) || GamePad__2.Down(PAD_R2) ||
            GamePad__2.Down(PAD_L1) || GamePad__2.Down(PAD_L2)) {
            mgPerformanceMeter(mgGetPerformanceMeterFlag() ^ 1);
        }
        if (MovieView->EndCheck() || GamePad__2.Down(PAD_START)) {
            MovieView->Term();
            MovieView->SwitchThread();
            MovieMode = MOVIE_VIEW_MODE_SELECT;
            MovieScene->StopBGM(0);
            Stack_ReadBuff.stack_used = 0;
            Stack_ReadBuff.lock = 0;
            if (MovieSpecialMode == MOVIE_SPECIAL_MODE_PROMO || MovieSpecialMode == MOVIE_SPECIAL_MODE_PROMO_TV) {
                ++MovieSpecialModeInfo[0];
                if (MovieSpecialModeInfo[0] < 4) {
                    char file_name[0x40];
                    MovieMode = MOVIE_VIEW_MODE_PLAY;
                    if (MovieSpecialMode == MOVIE_SPECIAL_MODE_PROMO) {
                        sprintf(file_name, "PROMO%d.PSS", MovieSpecialModeInfo[0]);
                    }
                    if (MovieSpecialMode == MOVIE_SPECIAL_MODE_PROMO_TV) {
                        sprintf(file_name, "PROMO%dTV.PSS", MovieSpecialModeInfo[0]);
                    }
                    MovieView->Load(file_name, &Stack_ReadBuff, 0x200, 0x1A0, true, false);
                    MovieView->Play("moviework");
                    MovieView->SwitchThread();
                    while (MovieView->IsStarted() == 0) {
                        MovieView->SwitchThread();
                    }
                } else {
                    MovieSpecialModeInfo[0] = 0;
                    MovieSpecialMode = MOVIE_SPECIAL_MODE_NONE;
                }
            }
            return 0;
        }
    }
    return 0;
}

// Static initialiser (.init)

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", tag_movie__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_786__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_843__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_844__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_1028__8__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_1029__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_1030__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_1031__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_1032__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_1033__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_1034__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_1035__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_1036__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movieviewlp", at_1037__5__DATA);

// Static initialiser table (.ctor)

// Small uninitialised data (.sbss)
INCLUDE_BSS(MovieScene, 0x4);
INCLUDE_BSS(MovieView, 0x4);
INCLUDE_BSS(RushWork__2, 0x4);
INCLUDE_BSS(performance_meter_flag, 0x4);
INCLUDE_BSS(MovieListNum, 0x4);
INCLUDE_BSS(MovieList, 0x4);
INCLUDE_BSS(MovieLine, 0x4);
INCLUDE_BSS(MovieSelect, 0x4);
INCLUDE_BSS(spi_MovieStack, 0x4);
INCLUDE_BSS(MovieSpecialMode, 0x4);
INCLUDE_BSS(MovieSpecialModeInfo, 0x8);
INCLUDE_BSS(MovieMode, 0x4);
INCLUDE_BSS(init_792, 0x4);
INCLUDE_BSS(init_795, 0x4);
INCLUDE_BSS(init_798, 0x4);
INCLUDE_BSS(init_801, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(DataBuffer__2, 0x30);
INCLUDE_BSS(Stack_ReadBuff__2, 0x30);
INCLUDE_BSS(buf0_791, 0x30);
INCLUDE_BSS(buf1_794, 0x30);
INCLUDE_BSS(dbuf0_797, 0x30);
INCLUDE_BSS(dbuf1_800, 0x30);
