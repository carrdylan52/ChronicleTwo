#pragma once

#include "common.h"
#include "editdata.hpp"
#include "mg_memory.hpp"

struct DownLoadEntry;
struct GeoStoneDmyCnt;

// The native announcement clamps unknown towns to 0. The sandbox wrapper
// clears its menu state instead, and binds all later UI flag writes privately.
extern int analyze_percent asm("Georama_AnalyzePercent");
extern int PartsMakeOkTableNum asm("Georama_MakeAvailableCount");
extern EditAnalyzeSrc *MenuEditAnalyzeSrc asm("Georama_AnalyzeSource");
extern short MenuEditAnalyzeDataSrcNum asm("Georama_AnalyzeCount");
extern EditAnalyzeDataSrc *MenuEditAnalyzeDataSrc[32] asm("Georama_AnalyzeEntries");
extern EditDataAnalyze *MenuAnalyzeData asm("Georama_AnalyzeState");
extern u16 MenuGeoStoneDownLoad_PartsNum asm("Georama_DownloadPartsCount");
extern u16 MenuGeoStoneDownLoad_Request asm("Georama_DownloadRequestCount");
extern u32 MenuGeoStoneDownLoadTime asm("Georama_DownloadTime");
extern DownLoadEntry *DownLoadInfo asm("Georama_DownloadFirst");
extern DownLoadEntry *DownLoadInfoNext asm("Georama_DownloadNext");
extern GeoStoneDmyCnt *MenuGeoStoneDmyCnt asm("Georama_DownloadCounter");
extern GeoStoneDmyCnt *MenuGeoStoneDmyCnt_Now asm("Georama_DownloadCounterNow");
extern signed char DownLoadMesMakeProgress asm("Georama_DownloadMessageProgress");

int PortRetailMakeDownLoadAnaunce(int, mgCMemory *, int *, int *, int *) asm("Georama_RetailAnnouncement");

// Retail: 0.85 * table[id]. The seam keeps the fit scale when a sandbox id's table entry is zero.
float PortGeoramaPreviewScale(int id, float fit_scale, float table_scale);
