#pragma once
#include <cstdint>
#include <filesystem>
#include <deque>
#include <string>
class CActiveMonster;
// Drains confirmed kills after every game frame, including outside the dungeon.
void IxionLedgerFrameBoundary();

// One counted kill of an installed encounter spawn. Only the place
// (room, DC2 floor, authored encounter slot) and the DC1 donor id identify
// the kill; the ledger id is added under the lock. session, generation and
// monster_id are process-local facts kept for logs: a relaunch, a save
// reload or a respawn changes them, and must not make a second delivery.
struct IxionLedgerKill {
    std::string room;
    int dungeon = -1;
    int floor = -1;
    int slot = 0;
    std::string donor; // empty for the native DC2 enemy
    std::string session;
    uint64_t generation = 0;
    int monster_id = -1;
};
std::string IxionLedgerKillKey(const IxionLedgerKill &kill);
// Adapt the saved roster identity without changing Ledger R2's e0:dc1-id encoding.
bool IxionLedgerKillFromRosterKey(const std::string &key, IxionLedgerKill &kill);
// Takes the ledger lock and appends the kill. retryable is set for a busy
// lock, an unreadable ledger or a failed commit; anything else is a refusal.
bool IxionLedgerPublishKill(const std::filesystem::path &ledger, const IxionLedgerKill &kill,
                            const std::filesystem::path &save_root, uint64_t created, uint64_t &seq, bool &added,
                            std::string &event, bool &retryable, std::string &error);

// Bounded retry of one pending publication on later frames. Retrying is safe
// because the key is stable: an append that did publish dedupes to added=0.
enum class IxionLedgerStep { kIdle, kPublished, kRetrying, kLost, kRefused };
struct IxionLedgerRetry {
    static constexpr int kMaxAttempts = 300;
    IxionLedgerKill kill;
    bool pending = false;
    int attempts = 0;
    uint64_t seq = 0;
    bool added = false;
    std::string event;
    std::string error;
    void Arm(const IxionLedgerKill &next) {
        kill = next;
        pending = true;
        attempts = 0;
    }
    IxionLedgerStep Step(const std::filesystem::path &ledger, const std::filesystem::path &save_root,
                         uint64_t created);
};

// Only ThinkHost's real count seam produces events. Capture the installed
// pointer+incarnation's immutable identity before NONE/cleanup; retirement is
// never a producer. Every authored roster spawn is Water-eligible.
void IxionLedgerNoteKillConfirmed(const CActiveMonster *monster);
struct IxionLedgerQueue {
    static constexpr size_t kCapacity = 32; // includes the active retry
    // Copies the event; no actor/room pointers survive into this queue.
    bool Enqueue(const IxionLedgerKill &kill);
    bool Confirm(const CActiveMonster *monster);
    size_t PendingCount() const { return queued.size() + (retry.pending ? 1 : 0); }
    IxionLedgerStep Step(const std::filesystem::path &ledger, const std::filesystem::path &save_root,
                         uint64_t created);
    // Result of the last Step, retained for diagnostics after completion.
    IxionLedgerRetry retry;
private:
    std::deque<IxionLedgerKill> queued;
};
