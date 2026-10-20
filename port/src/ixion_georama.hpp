#pragma once

#include <array>
#include <filesystem>
#include <string>

class CEditData;
class CEditMap;
class CVillagerPlaceInfo;

// The offline sandbox preparer creates a marker in an explicitly disposable save root.
bool IxionGeoramaRequested();
bool IxionGeoramaEnabled();
void IxionGeoramaPrepare(int map);
void IxionGeoramaArrival();
void IxionGeoramaStep();
bool IxionGeoramaSandboxPart(int id);
// Sandbox villagers: drops those whose house is not placed and moves the rest to the house NPC spot.
int IxionGeoramaFilterVillagers(int map_no, int *ids, CVillagerPlaceInfo **places, int found);
bool IxionGeoramaSaveSafe(const CEditMap &map);
bool IxionGeoramaRead(const std::filesystem::path &path, CEditData &data,
                      std::array<short, 128> &stock, std::string &error);
bool IxionGeoramaWrite(const std::filesystem::path &path, const CEditData &data,
                       const std::array<short, 128> &stock);
