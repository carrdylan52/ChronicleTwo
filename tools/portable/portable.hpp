#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace portable {
void                  CheckIso(const std::filesystem::path &path);
bool                  CurrentData(const std::filesystem::path &root, int version);
std::filesystem::path SuggestedIso(const std::filesystem::path &package);
void                  RememberIso(const std::filesystem::path &package, const std::filesystem::path &iso);
std::wstring          QuoteArgument(const std::wstring &argument);
std::wstring          CommandLine(const std::filesystem::path &program, const std::vector<std::wstring> &arguments);
} // namespace portable
