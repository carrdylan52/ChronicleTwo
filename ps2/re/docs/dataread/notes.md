# dataread: reverse-engineering notes

`GetPackFileNum` probes consecutive entries with `GetPackFile(pack, index, &name, &size)`
until the lookup fails, then returns the count. The output name and size are scratch values;
the loop shape in the matching source preserves the retail branch layout.

File access layer: device-prefixed paths, the DATA.DAT index, background read queue, file
cache, pack-file lookup. First-game counterpart: `chronicle/ps2/include/dataread.hpp` +
`ps2/src/dataread.cpp` (PAL branch is closest: linear `SearchFile`, no name tree). This game adds
devices (host/net/HDD), the file cache, current/top directories and the I/O error callback.

No classes are owned by this unit (`class_units.tsv` has none).

## Linkage (from the retail ELF, `readelf -s rom/pal/extracted/iso/SCES_511.90`)
- LOCAL functions (go in the `.cpp` as `static`, not in the header): `SearchFile(char*)`,
  `GetDevType(char*, char*)`, `ConvStr(char*)`, `GetFullPath(char*, char*)`,
  `CDRead(char*, u_int*, int*)`, `align_size(u_int, u_int)`, `GetNewFileCache()`,
  `EntryFileCache(char*, u_long128*, int)`, `SearchFileCache(char*)`.
- ALL data symbols are LOCAL (`static`): `TopDir`, `CurrentDir` (splitter calls it
  `CurrentDir__2`; the ELF name is `CurrentDir`), `DefaultFileDev`, `header_num`,
  `packfile_buff`, `data_sector`, `error_cb`, `old_vsync`, `start_vsync`, `CacheAddress`,
  `NowCacheAddress`, `FileCacheType`, `header_buff`, `bg_read_info`, `FileCache`.
  Hence the header has no `extern` data. Suggested definitions:
  - `static char TopDir[256];` `static char CurrentDir[256];` (.data, all zero: `= ""`).
  - `static int DefaultFileDev = FILE_DEV_CDROM;` (.sdata, value 1).
  - `static int header_num; static u_int *packfile_buff; static int data_sector;`
    `static int (*error_cb)(int); static int old_vsync; static int start_vsync;`
    `static u_long128 *CacheAddress; static u_long128 *NowCacheAddress; static int FileCacheType;`
    (.sbss, in this order).
  - `static u_char header_buff[0x50000]` (or DATA_HEADER-typed; code indexes it as 12-byte
    records and reads word 0 as an int), `static BG_READ_INFO bg_read_info[32]`,
    `static FILE_CACHE FileCache[16]`.
- `.bss` compiler statics `at_259` (0x100, LoadFileBG), `at_583` (0x100, LoadFile2),
  `at_845` (0x130? listed 0x130, WriteFile copies 0x100), `at_554` (0x10, GetFullPath) are
  zero-initialised local array initialisers copied to the stack: `char path[256] = "";`-style
  (8 x 32-byte copy loop). `at_554` is a 16-byte `char dev[16] = ""` style initialiser.

## Mangling
`P1` = `u_long128 *` (`unsigned __int128`), same as first game's `LoadFileBG__FPcP1Pi`.
`InitFileCache__FP1i` = (u_long128*, int); `EntryFileCache__FPcP1i` = (char*, u_long128*, int)
(asm: a0 name -> strcpy src, a1 -> +0 address, a2 -> +4 size). Ghidra/m2c mis-read both.

## Strings (.rodata)
at_183 "/", at_190 "", at_369 "error at %s\n", at_370 "LoadBG %s\n", at_438 "\DATA.DAT;1",
at_439 "cdrom0:\DATA.HD4;1", at_440 "File open error \"\"\n \n \n", at_441 "head size = %d/%d\n",
at_530 "host:", at_531 "host0:", at_532 "cdrom:", at_533 "net:", at_534 "psf0:" (sic; GetDevType's
HDD prefix), at_564 "pfs0:" (GetFullPath's HDD prefix), at_571 "File open error \"%s\"\n \n \n",
at_659 "file cache %s\n", at_660 "load %s\n", at_713 "Load %s\n", at_714 "%s %d %d\n".

## Enums (names are neutral, not retail)
- `FILE_DEV`: GetDevType returns -1 (no prefix / path[1]==':' single-letter drive), 0 host:/host0:,
  1 cdrom:, 2 net:, 3 psf0:. `DefaultFileDev` is 1 initially, 3 after ChangeHddFile.
  LoadFile2/LoadFileBG/WriteFile branch on these (2 -> LoadFileSocket/WriteFileSocket,
  1 -> DATA.DAT via SearchFile/CDRead, 3 -> sce* calls with error_cb, else sceOpen/sceRead).
  GetFullPath: prefix "" by default, "host:" for 0, "pfs0:" for 3; appends CurrentDir only when
  no prefix was given; lower-cases (ConvStr) for HDD.
- `LOAD_FILE_MODE` (LoadFile2 arg 4): 0 read whole file (113 callers pass 0); 1 size/existence
  only (LoadFileCacheBG); 2 open and return the descriptor (LoadFileBG, then ReadBG uses
  sceRead/sceIoctl/sceClose). Mode-2 open uses flags 0x8001 (SCE_RDONLY|SCE_NOWAIT).
- `FILE_CACHE_TYPE` (InitFileCache arg 2): 0 disabled (MainLoop, DeleteFileCache),
  1 downward (CScene::PreLoadVillager passes 1; NowCacheAddress starts at aligned address - 0x40
  and is decremented by align_size(size,0x800)/16 quads before each load), 2 upward.
  CacheAddress = align_size(address, 0x40) only when type is 1 or 2, else left 0 (disabled).

## Struct layouts
### DATA_HEADER (0xC) - DATA.HD4 record
SearchFile walks `header_buff` with stride 12, `strcasecmp(*(char**)rec, name)`. InitCDFile:
`header_num = *(int*)header_buff / 12` (first name offset = table size), then each word 0 is
rebased `+= header_buff` and '\\' -> '/'. +4 size (LoadFileBG/CDRead/LoadFile2 mode 1),
+8 sector relative to DATA.DAT (`+ data_sector`, which is the lsn of `\DATA.DAT;1`).
First game PAL record was 0x20 (name, 3 unused, offset, size, sector, sectors); this game's is 12.

### BG_READ_INFO (0x120) - bg_read_info[32] = 0x2400
Stride 0x48 words in all loops. +0 busy; +4 dev (LoadFileBG stores 1 for CD or the device;
ReadBG/BreakReadBG test `== 1`); +8 issued (CD: sceCdRead result; other: set 1 when sceRead is
issued; LoadFileBG sets 1 for cache hits); +0xC done; +0x10 name[256] (strcpy of
CurrentDir+name, strcasecmp in GetReadBGFile(char*)); +0x110 buffer; +0x114 size;
+0x118 sector (CD: header sector + data_sector) or fd (LoadFile2 mode-2 result; sceClose/sceIoctl
in ReadBG/BreakReadBG) - declared as an anonymous union; +0x11C sectors (size_to_sector(size)).
InitReadBG unrolled clear touches only +0 per slot. First game slot was name[128] with no dev/fd.
Note LoadFileBG copies the path into a 256-byte local, but `name` ends at +0x110, so 256 is right.

### FILE_CACHE (0x40) - FileCache[16] = 0x400
GetNewFileCache/SearchFileCache stride 0x40 (16 words). +0 address (free when 0);
+4 size; +8 ref_count (EntryFileCache sets 1, LoadFileCacheBG ++ on re-request, LoadFile2 -- on
read from cache); +0xC never touched (unk_0c); +0x10 name (strcpy/strcasecmp), 48 bytes to end.

### PACK_ENTRY (no size asserted; variable chain)
GetPackFile*: name at +0 (strcasecmp, first byte 0 ends), +0x40 data offset from entry,
+0x44 size, +0x48 next-entry offset from entry. Same as the first game's PACK_ENTRY.
GetPackFile(char*) strips everything up to the last '/'. GetPackFileExt matches the text after
the first '.' of each entry name.

## Return types
- `ChangeHddFile`: 0 if already HDD; else MountHDDFileSystem() result if <= 0, else 1.
  Caller stores as error code. `ChangeDefaultFile`: whether the device was HDD (bool as int).
- `SetCurrentDir`/`GetCurrentDir`/`ChangeDir`/`DivPathName*`: Ghidra shows strcpy's return in v0;
  no caller uses it -> void. `ReadBG`: same, void.
- `LoadFile` always 1 (Exit(0) on failure). `LoadFile2`: 0/1, mode 2 -> descriptor (host path
  returns -1 on open failure in mode 2).
- `SearchFileCache(char*, int*)` returns the cache address (+0) -> `u_long128 *`.
  Static `SearchFileCache(char*)` returns the FILE_CACHE entry.
- `GetDevType` returns FILE_DEV (int); `GetFullPath` returns the device too.
- `InitCDFile`: Ghidra shows printf result; treated as void.

## Unresolved
- Retail names of the four types/enums are unknown (DATA_HEADER, BG_READ_INFO, PACK_ENTRY from
  the first game; FILE_CACHE, FILE_DEV, LOAD_FILE_MODE, FILE_CACHE_TYPE neutral).
- `FILE_CACHE::unk_0c` never accessed.
- `packfile_buff` only zeroed (InitCDFile); type `u_int *` assumed from the first game.
- `size_to_sector` = ceil(size / 2048) with signed division (`size / 0x800 + (size % 0x800 != 0)`).

## C++ draft status
- All 39 functions have compiled C++ implementations and match retail. No function remains a
  guarded draft or an INCLUDE_ASM implementation.
- `LoadFile` and `InitCDFile` call the SDK `Exit(int)` as `Exit(0)`.
- File-local data definitions are unguarded. `header_buff` is static and has its 0x50000-byte
  BSS placeholder; the linked image preserves the retail data layout.
- `sceCdlFILE` is 0x24 bytes, including its ISO 9660 `flag` word. `InitCDFile` retries
  `sceCdSearchFile` in an inner loop before `sceCdSync`. Its name offsets are rebased through an
  integer address addition.
- `LoadFile2` reuses `dev` and `result` as file descriptors. A separate `fd` local differs in
  13 of 248 words (0x3D4 bytes against retail's 0x3E0).
- `LoadFileCacheBG` moves `NowCacheAddress` by `align_size(size, 2048) / 16` quads before
  queuing in either direction. `GetPackFile(char*)` returns null for a null or empty name.
  `DivPathName` treats a slash at index 0 as no directory and copies the whole path into the name.
- Variable integer division retains the retail division-by-zero trap.
