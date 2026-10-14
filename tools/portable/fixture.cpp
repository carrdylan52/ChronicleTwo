#include <nlohmann/json.hpp>
#include <windows.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

#include "kazarov/data_version.hpp"

namespace fs = std::filesystem;

std::string Utf8(const fs::path &path) {
    const auto bytes = path.u8string();
    return {reinterpret_cast<const char *>(bytes.data()), bytes.size()};
}

int wmain(int argc, wchar_t **argv) {
    nlohmann::json arguments = nlohmann::json::array();
    for (int i = 1; i < argc; ++i) {
        arguments.push_back(Utf8(fs::path(argv[i])));
    }
    const bool    extractor = fs::path(argv[0]).filename() == L"dc2data.exe";
    std::ofstream calls(fs::current_path() / "calls.jsonl", std::ios::app);
    calls << nlohmann::json{
                 {"program",   extractor ? "extractor" : "game"},
                 {"arguments", arguments                       },
                 {"cwd",       Utf8(fs::current_path())        },
                 {"pid",       GetCurrentProcessId()           }
    }
                 .dump()
          << '\n';
    calls.close();
    if (extractor) {
        if (GetEnvironmentVariableW(L"CT_PORTABLE_FIXTURE_FAIL", nullptr, 0)) {
            return 7;
        }
        if (argc != 4 || std::wstring(argv[1]) != L"extract") {
            return 2;
        }
        const fs::path data(argv[3]), root = data.parent_path();
        fs::create_directories(data);
        fs::create_directories(root / "port_data");
        std::ofstream(data / "manifest.json") << "{}";
        std::ofstream(root / "port_data" / "manifest.json") << "{}";
        std::ofstream(root / "data.json") << nlohmann::json{
            {"schema_version", 1                   },
            {"data_version",   KAZAROV_DATA_VERSION}
        };
    } else {
        for (int i = 1; i < argc; ++i) {
            if (std::wstring(argv[i]) == L"--fixture-sleep") {
                std::this_thread::sleep_for(std::chrono::seconds(4));
            }
        }
        if (GetEnvironmentVariableW(L"CT_PORTABLE_FIXTURE_GAME_FAIL", nullptr, 0)) {
            std::puts("fixture game error: retain this log");
            return 4;
        }
        std::ofstream(fs::current_path() / "game.done") << "game completed";
    }
    return 0;
}
