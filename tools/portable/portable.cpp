#include "portable.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cwctype>
#include <fstream>
#include <map>
#include <stdexcept>

namespace portable {
namespace fs = std::filesystem;

namespace {
constexpr std::uint64_t Sector = 2048;

std::uint32_t BothEndian(const unsigned char *bytes) {
    const auto little = std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8) |
                        (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
    const auto big = std::uint32_t(bytes[7]) | (std::uint32_t(bytes[6]) << 8) |
                     (std::uint32_t(bytes[5]) << 16) | (std::uint32_t(bytes[4]) << 24);
    if (little != big) {
        throw std::runtime_error("The disc image has inconsistent ISO directory records.");
    }
    return little;
}

struct Entry {
    std::uint64_t offset;
    std::uint32_t size;
    bool          directory;
};

std::vector<unsigned char> Read(std::ifstream &input, std::uint64_t total, std::uint64_t offset, std::uint32_t size) {
    if (offset > total || size > total - offset) {
        throw std::runtime_error("The disc image is incomplete or damaged.");
    }
    std::vector<unsigned char> result(size);
    input.seekg(static_cast<std::streamoff>(offset));
    input.read(reinterpret_cast<char *>(result.data()), size);
    if (!input) {
        throw std::runtime_error("Could not read the disc image. Check that the file is available locally.");
    }
    return result;
}

nlohmann::json ReadJson(const fs::path &path) {
    std::error_code error;
    const auto      size = fs::file_size(path, error);
    if (error || size > 1024 * 1024) {
        return nullptr;
    }
    std::ifstream input(path);
    if (!input) {
        return nullptr;
    }
    return nlohmann::json::parse(input, nullptr, false);
}
} // namespace

void CheckIso(const fs::path &path) {
    std::error_code error;
    const auto      total = fs::file_size(path, error);
    std::ifstream   input(path, std::ios::binary);
    if (error || !input) {
        throw std::runtime_error("Could not open this ISO. Choose a disc image stored on this computer.");
    }
    std::vector<unsigned char> descriptor;
    for (unsigned sector = 16; sector < 80; ++sector) {
        auto block = Read(input, total, sector * Sector, Sector);
        if (!std::equal(block.begin() + 1, block.begin() + 6, "CD001") || block[6] != 1) {
            throw std::runtime_error("This file is not a supported ISO disc image. Choose an uncompressed .iso file.");
        }
        if (block[0] == 1) {
            descriptor = std::move(block);
            break;
        }
        if (block[0] == 255) {
            break;
        }
    }
    if (descriptor.empty()) {
        throw std::runtime_error("This disc image has no ISO filesystem.");
    }
    if (descriptor[128] != 0 || descriptor[129] != 8 || descriptor[130] != 8 || descriptor[131] != 0) {
        throw std::runtime_error("This disc image uses an unsupported sector size.");
    }
    const auto volumeSize = std::uint64_t(BothEndian(descriptor.data() + 80)) * Sector;
    if (volumeSize > total || volumeSize < 17 * Sector) {
        throw std::runtime_error("The disc image is incomplete or damaged.");
    }
    const auto *record = descriptor.data() + 156;
    if (record[0] < 34 || !(record[25] & 2)) {
        throw std::runtime_error("The disc image has no valid root directory.");
    }
    const auto rootOffset = std::uint64_t(BothEndian(record + 2)) * Sector;
    const auto rootSize = BothEndian(record + 10);
    if (!rootSize || rootSize > 16 * 1024 * 1024) {
        throw std::runtime_error("The disc image has an invalid root directory.");
    }
    const auto                   root = Read(input, volumeSize, rootOffset, rootSize);
    std::map<std::string, Entry> entries;
    for (std::size_t pos = 0; pos < root.size();) {
        const unsigned length = root[pos];
        if (!length) {
            pos = (pos / Sector + 1) * Sector;
            continue;
        }
        if (length < 34 || length > root.size() - pos || length > Sector - pos % Sector) {
            throw std::runtime_error("The disc image has a damaged directory.");
        }
        record = root.data() + pos;
        const unsigned nameSize = record[32];
        if (nameSize > length - 33) {
            throw std::runtime_error("The disc image has a damaged filename.");
        }
        std::string name(reinterpret_cast<const char *>(record + 33), nameSize);
        if (const auto version = name.find(';'); version != std::string::npos) {
            name.resize(version);
        }
        const Entry entry{std::uint64_t(BothEndian(record + 2)) * Sector, BothEndian(record + 10), bool(record[25] & 2)};
        if (entry.offset > volumeSize || entry.size > volumeSize - entry.offset) {
            throw std::runtime_error("The disc image has an incomplete file.");
        }
        if (name != std::string(1, '\0') && name != std::string(1, '\1')) {
            if (!entries.emplace(name, entry).second) {
                throw std::runtime_error("The disc image has duplicate root files.");
            }
        }
        pos += length;
    }
    const auto elf = entries.find("SCES_511.90");
    const auto config = entries.find("SYSTEM.CNF");
    if (elf == entries.end() || config == entries.end() || elf->second.directory || config->second.directory) {
        throw std::runtime_error("Choose the PAL European Dark Chronicle disc (SCES-51190). Other regions and Dark Cloud 1 are not supported.");
    }
    if (config->second.size > 16 * 1024 || elf->second.size < 4) {
        throw std::runtime_error("The Dark Chronicle boot files are damaged.");
    }
    const auto        boot = Read(input, volumeSize, config->second.offset, config->second.size);
    const std::string bootText(boot.begin(), boot.end());
    if (bootText.find("SCES_511.90") == std::string::npos) {
        throw std::runtime_error("The disc's boot configuration does not identify PAL Dark Chronicle.");
    }
    const auto magic = Read(input, volumeSize, elf->second.offset, 4);
    if (magic != std::vector<unsigned char>{0x7f, 'E', 'L', 'F'}) {
        throw std::runtime_error("The Dark Chronicle executable in this disc image is damaged.");
    }
    for (const auto *name : {"DATA.HD4", "DATA.DAT", "SOUND.HD3", "SOUND.DAT", "MOVIE"}) {
        const auto found = entries.find(name);
        if (found == entries.end() || found->second.directory != (std::string(name) == "MOVIE") || !found->second.size) {
            throw std::runtime_error("This Dark Chronicle disc image is missing required game files.");
        }
    }
}

bool CurrentData(const fs::path &root, int version) {
    std::error_code error;
    if (!fs::is_regular_file(root / "data" / "manifest.json", error) ||
        !fs::is_regular_file(root / "port_data" / "manifest.json", error)) {
        return false;
    }
    const auto marker = ReadJson(root / "data.json");
    return marker.is_object() && marker.contains("schema_version") && marker["schema_version"].is_number_integer() &&
           marker["schema_version"] == 1 && marker.contains("data_version") && marker["data_version"].is_number_integer() &&
           marker["data_version"] == version;
}

fs::path SuggestedIso(const fs::path &package) {
    const auto settings = ReadJson(package / "userdata" / "portable.json");
    if (settings.is_object() && settings.contains("iso") && settings["iso"].is_string()) {
        try {
            const auto utf8 = settings["iso"].get<std::string>();
            auto       path = fs::path(std::u8string(reinterpret_cast<const char8_t *>(utf8.data()), utf8.size()));
            if (path.is_relative()) {
                path = package / path;
            }
            CheckIso(path);
            return path;
        } catch (const std::exception &) {
        }
    }
    std::error_code error;
    fs::path        candidate;
    for (fs::directory_iterator entry(package, error), end; !error && entry != end; entry.increment(error)) {
        auto extension = entry->path().extension().wstring();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t c) { return std::towlower(c); });
        if (extension != L".iso") {
            continue;
        }
        try {
            CheckIso(entry->path());
            if (!candidate.empty()) {
                return {};
            }
            candidate = entry->path();
        } catch (const std::exception &) {
        }
    }
    return candidate;
}

void RememberIso(const fs::path &package, const fs::path &iso) {
    auto stored = iso.lexically_relative(package);
    if (stored.empty() || *stored.begin() == "..") {
        stored = iso;
    }
    const auto           path = stored.generic_u8string();
    const nlohmann::json settings{
        {"schema_version", 1},
        {"iso", std::string(reinterpret_cast<const char *>(path.data()), path.size())}
    };
    std::ofstream output(package / "userdata" / "portable.json", std::ios::trunc);
    if (output) {
        output << settings.dump(2) << '\n';
    }
}

std::wstring QuoteArgument(const std::wstring &argument) {
    std::wstring quoted = L"\"";
    std::size_t  slashes = 0;
    for (const auto c : argument) {
        if (c == L'\\') {
            ++slashes;
            continue;
        }
        quoted.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        quoted += c;
        slashes = 0;
    }
    quoted.append(slashes * 2, L'\\');
    return quoted + L'"';
}

std::wstring CommandLine(const fs::path &program, const std::vector<std::wstring> &arguments) {
    auto command = QuoteArgument(program.wstring());
    for (const auto &argument : arguments) {
        command += L" " + QuoteArgument(argument);
    }
    return command;
}
} // namespace portable
