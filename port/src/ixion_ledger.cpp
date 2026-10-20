#include "ixion_delivery_lock.hpp"
#include "ixion_delivery.hpp"
#include "ixion_ledger.hpp"

#include "common.h"
#include "ixion_room.hpp"
#include "platform/files.hpp"
#include "platform/paths.hpp"
#include <chrono>
#include <charconv>
#include <cstdio>
#include <cstdlib>

namespace {
std::string KeyPart(const std::string &text) {
    std::string out = text.empty() ? std::string("none") : text.substr(0, 48);
    for (char &c : out) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        if (!ok) c = '_';
    }
    return out;
}
} // namespace

std::string IxionLedgerKillKey(const IxionLedgerKill &kill) {
    return KeyPart(kill.room) + ":" + std::to_string(kill.dungeon) + ":" + std::to_string(kill.floor) + ":e" +
           std::to_string(kill.slot) + ":" + (kill.donor.empty() ? std::string("native") : "dc1-" + KeyPart(kill.donor));
}

bool IxionLedgerKillFromRosterKey(const std::string &key, IxionLedgerKill &kill) {
    // Room keys contain exactly room:dungeon:floor:spawnNN:donor. Parse saved
    // metadata, never the current roster option or a released donor's state.
    std::string parts[5];
    size_t begin = 0;
    for (int i = 0; i < 5; ++i) {
        const size_t end = key.find(':', begin);
        if ((i < 4 && end == std::string::npos) || (i == 4 && end != std::string::npos)) return false;
        parts[i] = key.substr(begin, end == std::string::npos ? end : end - begin);
        begin = end + 1;
    }
    auto number = [](const std::string &text, int &out) {
        const auto result = std::from_chars(text.data(), text.data() + text.size(), out);
        return result.ec == std::errc{} && result.ptr == text.data() + text.size() && out >= 0;
    };
    IxionLedgerKill adapted = kill;
    if (parts[0].empty() || parts[4].empty() || parts[3].substr(0, 5) != "spawn" ||
        !number(parts[1], adapted.dungeon) || !number(parts[2], adapted.floor) ||
        !number(parts[3].substr(5), adapted.slot)) return false;
    adapted.room = parts[0];
    adapted.donor = parts[4];
    kill = adapted;
    return true;
}

bool IxionLedgerPublishKill(const std::filesystem::path &path, const IxionLedgerKill &kill,
                            const std::filesystem::path &save_root, uint64_t created, uint64_t &seq, bool &added,
                            std::string &event, bool &retryable, std::string &error) {
    added = false;
    ixion::delivery::Lock lock(path);
    if (!lock.ok()) {
        retryable = true;
        error = "ledger busy or inaccessible";
        return false;
    }
    auto write = [](const auto &p, const std::string &text) {
        return FilesWrite(p, text.data(), text.size(), true) == FilesResult::kWritten;
    };
    return ixion::delivery::AppendWorldKill(path, IxionLedgerKillKey(kill), save_root, created, write, seq, added,
                                            event, retryable, error);
}

IxionLedgerStep IxionLedgerRetry::Step(const std::filesystem::path &ledger, const std::filesystem::path &save_root,
                                       uint64_t created) {
    if (!pending) return IxionLedgerStep::kIdle;
    ++attempts;
    bool retryable = false;
    if (IxionLedgerPublishKill(ledger, kill, save_root, created, seq, added, event, retryable, error)) {
        pending = false;
        return IxionLedgerStep::kPublished;
    }
    if (!retryable) {
        pending = false;
        std::fprintf(stderr, "ixion ledger delivery REFUSED: %s\n", error.c_str());
        return IxionLedgerStep::kRefused;
    }
    if (attempts >= kMaxAttempts) {
        pending = false;
        return IxionLedgerStep::kLost;
    }
    return IxionLedgerStep::kRetrying;
}

namespace {
IxionLedgerQueue &KillQueue() {
    static IxionLedgerQueue queue;
    return queue;
}
}

bool IxionLedgerQueue::Enqueue(const IxionLedgerKill &kill) {
    if (PendingCount() >= kCapacity) {
        std::fprintf(stderr, "ixion ledger LOST delivery: kill queue overflow capacity=%zu key=%s\n",
                     kCapacity, IxionLedgerKillKey(kill).c_str());
        return false;
    }
    queued.push_back(kill);
    return true;
}

bool IxionLedgerQueue::Confirm(const CActiveMonster *monster) {
    IxionLedgerKill kill;
    return IxionRoomCaptureLedgerKill(monster, kill) && Enqueue(kill);
}

void IxionLedgerNoteKillConfirmed(const CActiveMonster *monster) {
    const char *setting = std::getenv("IXION_LEDGER");
    if (setting && *setting) KillQueue().Confirm(monster);
}

IxionLedgerStep IxionLedgerQueue::Step(const std::filesystem::path &ledger,
                                       const std::filesystem::path &save_root, uint64_t created) {
    if (!retry.pending) {
        if (queued.empty()) return IxionLedgerStep::kIdle;
        retry.Arm(queued.front());
        queued.pop_front();
    }
    return retry.Step(ledger, save_root, created);
}

void IxionLedgerFrameBoundary() {
    static const char *setting = std::getenv("IXION_LEDGER");
    if (!setting || !*setting) return;
    const uint64_t created = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    auto &queue = KillQueue();
    const IxionLedgerStep step = queue.Step(PathsFromUtf8(setting), PathsSaveRoot(), created);
    const IxionLedgerRetry &retry = queue.retry;
    switch (step) {
    case IxionLedgerStep::kPublished:
        std::fprintf(stderr, "ixion ledger delivery seq=%llu item=145 source=dc2 native_dead=1 generation=%llu enemy=%d added=%d attempts=%d key=%s\n",
                     static_cast<unsigned long long>(retry.seq), static_cast<unsigned long long>(retry.kill.generation),
                     retry.kill.monster_id, retry.added ? 1 : 0, retry.attempts, retry.event.c_str());
        break;
    case IxionLedgerStep::kRetrying:
        if (retry.attempts == 1 || retry.attempts % 60 == 0)
            std::fprintf(stderr, "ixion ledger delivery pending: %s; retrying next frame (attempt %d/%d)\n",
                         retry.error.c_str(), retry.attempts, IxionLedgerRetry::kMaxAttempts);
        break;
    case IxionLedgerStep::kLost:
        std::fprintf(stderr, "ixion ledger LOST delivery after %d attempts: %s\n", retry.attempts, retry.error.c_str());
        break;
    case IxionLedgerStep::kRefused:
        break;
    case IxionLedgerStep::kIdle:
        break;
    }
}
