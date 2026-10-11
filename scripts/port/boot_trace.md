# Boot trace (from source)

The PAL boot path from `main` to the title screen, read from `ps2/src`, one line per function in call
order. Depth is shown by `·`. The last column names the hardware-inventory rows the step reaches
(`[unit:function]`, as in `docs/port/HARDWARE_INVENTORY.md`) and the SDK symbols it calls, with their
ledger replacement where it is not `stub`. `scripts/port/inventory_annotate.py` reads the `[unit:function]`
tokens: a row's `trace_position` is `boot:<#>` of the first step naming it. Loops are written once, at
their first iteration. Replace this with the `--trace` log at M1.

Two corrections to the plan's Ground truth: the game's `main` calls `GetThreadId` and
`ChangeThreadPriority` before `init()`, so a trace entering the game's `main` stops there, not at
`sceDmaReset`; and `L5LOGO.PSS` is loaded by `TitleCopyRightStep` (`title.cpp:2499`), a title-loop phase
after the memory-card check, not by `TitleBootInit`. `SCElogoFade(0)` first runs a language-select loop
(`TitleLangSelInit/Key/Draw` between `mgBeginFrame` and `mgEndFrame`) before the logo fade.

Step 0 is the host's: the PS2 ran the static constructors in `mwInit`, inside `init()`; the host runs
them before `main`. The M1 run showed which inventory rows they reach.

| # | call | source | reaches |
|---|---|---|---|
| 0 | static constructors: `CAutoMapGen::CAutoMapGen`, `CRandomCircle::CRandomCircle`, `CScene::CScene`, `CAquarium::CAquarium` → `CAquarium::Clear` → `sndInitPort` → `sndSeAllStop` → `CSound::Stop` | `dng_main.cpp:195,197`, `menuaqua.cpp:3643,3690`, `snd_mngr.cpp:489,846`, `sound.cpp:667` | [mg_drawenv:mgCDrawEnv::Initialize] [mg_math:mgUnitMatrix] [mg_math:mgZeroVectorW] [snd_mngr:sndWaitSema] [snd_mngr:sndSignalSema] [ezmidi:ezMidi] |
| 1 | `main` | `main.cpp:138` | [main:main]; `GetThreadId`, `ChangeThreadPriority` |
| 2 | · `init` (static) | `main.cpp:79` | [main:init]; `sceDmaReset`, `sceGsResetPath`, `sceGsResetGraph`, `sceGsSyncVCallback` |
| 3 | · · `ClearScreen` (static) | `main.cpp:50` | [main:ClearScreen]; `sceGsSetDefDBuff`, `FlushCache` (adapter), `sceGsSyncV`, `sceGsSwapDBuff`, `sceGsSyncPath` |
| 4 | · · `mwInit` | MW runtime | ledger `kazarov-runtime` |
| 5 | · · SDK start-up | `main.cpp:87-131` | `sceSifInitRpc` (adapter), `sceCdInit`, `sceCdMmode`, `sceSifRebootIop`, `sceSifSyncIop`, `sceFsReset`, `sceSifLoadModule` ×10 |
| 6 | · · `InitCDFile` | `dataread.cpp:499` | [dataread:InitCDFile]; `sceCdSearchFile`, `sceCdSync`, `sceOpen`, `sceLseek`, `Exit__2` on error |
| 7 | · `MainLoop` | `mainloop.cpp:462` | [mainloop:MainLoop] |
| 8 | · · `CSaveData::Initialize` | `savedata.cpp:18` | |
| 9 | · · `CGamePad::Init` | `gamepad.cpp:57` | [gamepad:CGamePad::Init]; `scePadInit`, `scePadPortOpen`, `sceGsSyncV` |
| 10 | · · `InitFileCache` | `dataread.cpp:916` | |
| 11 | · · `mgInit` | `mglib.cpp:409` | [mglib:mgInit] [mg_drawenv:mgCDrawEnv::Initialize] [mglib:mgGetVuProgPacket]; `Vu_progmain`, `My_dma_start0` |
| 12 | · · `mgInitVSyncCallBack` | `mglib.cpp:340` | |
| 13 | · · `SCElogoFade(0)` | `nowload.cpp:640` | [nowload:SCElogoFade] |
| 14 | · · · `mgInitVif1Packet` | `mglib.cpp:560` | [mglib:mgInitVif1Packet] |
| 15 | · · · `mgSetPacketBuffer`, `mgSetDataBuffer` | `mglib.cpp:582,591` | |
| 16 | · · · `mgCTextureManager::SetTableBuffer`, `::Initialize`, `mgGetTopVRAMAddress` | `mg_texture.cpp:471,511`, `mglib.cpp:620` | [mglib:mgGetTopVRAMAddress] [mg_texture:mgCTextureManager::Initialize] (review rows: VRAM bookkeeping) |
| 17 | · · · `CGamePad::WaitEnable` | `gamepad.cpp:306` | [gamepad:CGamePad::WaitEnable] [gamepad:read_pad]; `scePadGetState`, `sceGsSyncV` |
| 18 | · · · `CGamePad::UpDate` | `gamepad.cpp:336` | [gamepad:CGamePad::UpDate] [gamepad:pad_button_read] [gamepad:SwitchGamePadThread]; `scePadRead`, `RotateThreadReadyQueue` |
| 19 | · · · `TitleLangSelInit` | `title.cpp:3208` | [dataread:LoadFile2] [dataread:CDRead] |
| 20 | · · · `mgBeginFrame` (language-select loop) | `mglib.cpp:637` | [mglib:mgBeginFrame] [mglib:mgBeginPacket] [mglib:mgSetPkTextureRepeat] [mglib:mgSetPkFrameBuffer] [mglib:mgSetPkClearScreen] [mglib:mgFlushRenderInfo] |
| 21 | · · · `TitleLangSelKey` | `title.cpp:3228` | |
| 22 | · · · `TitleLangSelDraw` → `DrawMenuFillBox` | `title.cpp:3289`, `menudraw.cpp:1981` | [mg_texture:mgCTextureManager::ReloadTexture] [mg_drawprim:mgCDrawPrim::Begin2] [mg_drawprim:mgCDrawPrim::BeginDma] [mg_drawprim:mgCDrawPrim::Color] [mg_drawprim:mgCDrawPrim::Vertex] [mg_drawprim:mgCDrawPrim::End2] |
| 23 | · · · `mgEndFrame` | `mglib.cpp:754` | [mglib:mgEndFrame] [mglib:mgWaitFrame] [mglib:mgEndPacket] [mglib:WaitVSync] [mglib:mgSendPacket] [mglib:VSyncCallBack]; `sceDmaGetChan`, `sceDmaSend`, `sceGsSyncPath`, timer `0x10000000` |
| 24 | · · · `LoadFile2("title/title%d.img")`, `mgCTextureManager::EnterIMGFile` | `nowload.cpp:679`, `mg_texture.cpp:1026` | [mg_texture:mgCTextureManager::EnterTexture] |
| 25 | · · · logo fade-in, 65 frames of one textured sprite | `nowload.cpp:691-730` | [mg_drawprim:mgCDrawPrim::Texture] |
| 26 | · · `CGamePad::WaitEnable`, `sceGsSyncV`, `CGamePad::UpDate` ×4 | `mainloop.cpp:505-518` | |
| 27 | · · `mgSetUserVuProg`, `mgSetUserVuProgAdr(0, Vu_prog_wtr)` | `mainloop.cpp:522-523` | `Vu_prog_wtr` (vu-data of [mainloop:MainLoop]) |
| 28 | · · `InitPadTable`, `CGameItemDataManage::LoadData`, `LoadItemSystemMes` | `mainloop.cpp:524-526` | [dataread:LoadFile] |
| 29 | · · `LoadSystemMes`, `CreateSystemMes`, `LoadFontTexture`, `LoadGaijiImg`, `LoadFontTblBin`, `LoadFontTex2Img`, `LoadNPCCfg`, `LoadGameInfo` | `mainloop.cpp:527-535` | file loads only |
| 30 | · · `InitPauseData`, `InitSaveData`, `LoadMonsterLanguage`, `LanguageEquipChange`, `DebugGetItem` | `mainloop.cpp:536-546` | |
| 31 | · · `sndInitMngr` | `snd_mngr.cpp:449` | [snd_mngr:sndInitMngr]; `CreateSema` |
| 32 | · · · `CSound::Init` | `sound.cpp:208` | [sound:CSound::Init]; `sceSifInitIopHeap`, `sceSifAllocSysMemory`, `sceMSIn_Init` |
| 33 | · · · · `ezBgmInit`, `ezMidiInit` | `ezbgm.cpp:24`, `ezmidi.cpp:26` | [ezbgm:ezBgmInit] [ezmidi:ezMidiInit]; `sceSifBindRpc` |
| 34 | · · · · `set_spu` | `sound.cpp:118` | [sound:set_spu]; `sceSdRemoteInit`, `sceSdRemote` |
| 35 | · · · · `ezMidi` | `ezmidi.cpp:52` | [ezmidi:ezMidi]; `sceSifCallRpc` |
| 36 | · · · `sndSetMasterVol` → `CSound::SetMasterVol` | `snd_mngr.cpp:609`, `sound.cpp:683` | [sound:CSound::SetMasterVol] |
| 37 | · · `CScene::InitSnd` | `scenesnd.cpp:32` | |
| 38 | · · `LoadFile2("snd2/SY_000.snd")`, `sndLoadSound` | `mainloop.cpp:566-567`, `snd_mngr.cpp:949` | [snd_mngr:sndWaitSema] [snd_mngr:sndSignalSema] [sound:CSound::LoadHdBd2] [sound:TransHdBd] [ezmidi:ezTransToIOP2] [sound:CSound::LoadSeq] |
| 39 | · · `LoadSndRevInfo`, `LoadSndFileInfo`, `LoadHelpMes`, `LoadMapName`, `LoadEditAnalyzeData`, `LoadFilePictureName`, `CFont::Init` | `mainloop.cpp:570-583` | file loads only |
| 40 | · · `CreateGamePadThread` | `gamepad.cpp:821` | [gamepad:CreateGamePadThread]; `CreateThread`, `StartThread` |
| 41 | · · `LoadGameConfig`, `SetIoErrCallBack` | `mainloop.cpp:587-588` | |
| 42 | · · `SCElogoFade(1)` | `nowload.cpp:640` | `sceGsSyncV` ×65, logo fade-out frames (steps 20-25) |
| 43 | · · `sndSeAllStop`, `sndDeletePort` | `snd_mngr.cpp:823,1097` | [sound:CSound::DEL_PORT] |
| 44 | · · `LoopInit[LOOP_TITLE]` → `TitleInit` | `title.cpp:606` | |
| 45 | · · · `mgInitFont` | `mglib.cpp:1998` | [mglib:mgInitFont]; `sceDevConsInit`, `sceDevConsOpen` |
| 46 | · · · `CScene::Initialize`, `mgInitVif1Packet`, `mgSetPacketBuffer`, `mgSetDataBuffer`, `SetTextureTable` | `title.cpp:612-662` | |
| 47 | · · · `TitleBootInit` (static) | `title.cpp:681` | |
| 48 | · · · · `CMemoryCardManager::Initialize` | `memcard.cpp:206` | |
| 49 | · · · · `CScene::LoadMap` → `CScene::LoadMapFromMemory` | `sceneload.cpp:501,305` | [sceneload:CScene::LoadMapFromMemory]; timer `0x10000000` |
| 50 | · · · · `EnterIMGFile`, `EnterTexture("water_work")`, `LoadFileMenu("save.pac")`, `GetPackFile` | `title.cpp:731-760` | |
| 51 | · · · · `EnterTexture("moviework")`, `LoadFile2("meswin/pushstart.img")`, `ReLoadFontTexture`, `LoadFileMenu("men0.pac")` | `title.cpp:762-791` | |
| 52 | · · · · `CActionChara::Initialize`, `CScene::LoadSound`, `sndWaitTransBd`, `sndLoadSound` | `title.cpp:793-803` | |
| 53 | · · · · `CMemoryCardManager::InitForMC` | `memcard.cpp:267` | [memcard:CMemoryCardManager::InitForMC]; `sceMcInit` |
| 54 | · · · · `TitleMCCheckInit` | `title.cpp:2205` | |
| 55 | · · title frame: `mgBeginFrame`, `TitleLoop`, `sndStep`, `CScene::StepSnd`, `CGamePad::UpDate`, frame border, `mgEndFrame` | `mainloop.cpp:680-702` | |
| 56 | · · · `TitleLoop` (memory-card check) → `TitleMCCheckKey` → `CMemoryCardManager::Step` | `title.cpp:870`, `memcard.cpp:500` | [memcard:CMemoryCardManager::SearchMcType]; `sceMcGetInfo`, `sceMcSync` |
| 57 | · · · `TitleDraw` → `TitleMCCheckDraw` | `title.cpp:1258,2403` | |
| 58 | · · · check done: `CMemoryCardManager::FinishForMC` | `memcard.cpp:288` | [memcard:CMemoryCardManager::FinishForMC]; `sceMcEnd` |
| 59 | · · · `TitleCopyRightStep` (copyright fade) | `title.cpp:2438` | |
| 60 | · · · · `mgCTextureManager::ReloadTexture(0x43)`, `DrawMenuFillBox` | `title.cpp:2492-2498` | |
| 61 | · · · · `CMovie::Load("L5LOGO.PSS")` | `movie.cpp:193` | [movie:CMovie::Load] [movie:CMovie::videoDecCreate] [movie:audioDecCreate] [movie:CMovie::videoDecSetStream] [movie:strFileOpen] [movie:strFileRead] [movie:viBufCreate] [movie:viBufReset]; `sceMpegInit`, `sceSdRemote`, `sceCdStInit` |
| 62 | · · · · `CMovie::Play("moviework")` | `movie.cpp:287` | [movie:CMovie::Play]; `CreateThread`, `AddIntcHandler`, `AddDmacHandler` |
| 63 | · · · · `CMovie::SwitchThread` until started | `movie.cpp:325` | [movie:switchThread] |
| 64 | · · · · movie threads and handlers | `movie.cpp:499-1563` | [movie:stepMain] [movie:decBs0] [movie:videoCallback] [movie:vblankHandler] [movie:viBufAddDMA] [movie:viBufBeginPut] [movie:viBufEndPut] [movie:viBufGetTs] [movie:viBufPutTs] [movie:audioDecSendToIOP] [movie:sendToIOP] [movie:setImageTag] [movie:startDisplay] [movie:voBufIncCount] |
| 65 | · · · `TitleCopyRightDraw` → `CMovie::Term` at the end | `title.cpp:2543`, `movie.cpp:338` | [movie:CMovie::Term] [movie:CMovie::videoDecFlush] [movie:CMovie::videoDecDelete] [movie:audioDecDelete] [movie:strFileClose] [movie:viBufDelete] |
| 66 | · · · `TitleModeInit`, title screen frames | `title.cpp:1434,1836` | title screen reached |
