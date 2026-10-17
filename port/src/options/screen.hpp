#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "gfx/gfx.hpp"

namespace options {

// The retail Options frame's dark interior, inside its gold border and left of its scroll bar.
constexpr int kPanelLeft = 72;
constexpr int kPanelTop = 88;
constexpr int kPanelRight = 440;
constexpr int kPanelBottom = 326;
// The tabs head the frame's interior, so they stay on screen at every offered UI scale.
constexpr int kTabLeft = 78;
constexpr int kTabRight = 432;
constexpr int kTabTop = 92;
constexpr int kTabHeight = 22;
// Rows use the retail list's 24-unit pitch; longer pages scroll as the retail list does.
constexpr int kVisibleRows = 8;
constexpr int kRowY = 120;
constexpr int kRowStep = 24;
constexpr int kLeft = 78;
constexpr int kRight = 432;
constexpr int kValueX = 268;
// The frame's scroll bar, inside its right-hand border.
constexpr int kScrollX = 446;
constexpr int kScrollTop = 88;
constexpr int kScrollHeight = 237;
// Back, the shortcut plate and Restore binding sit on the parchment between the frame and the help box.
constexpr int kButtonTop = 343;
constexpr int kButtonHeight = 27;
constexpr int kBackLeft = kPanelLeft;
constexpr int kBackWidth = 104;
constexpr int kHintLeft = kBackLeft + kBackWidth + 6;
constexpr int kResetLeft = kHintLeft;
// The help box holds two wrapped description lines, as the retail one does.
constexpr int kHelpX = 53;
constexpr int kHelpWidth = 410;
constexpr int kHelpLines = 2;
constexpr int kHelpSize = 14;
constexpr int kMaxPages = 8;
// Matches port_settings::PcOverlayPageNames() ("Bindings" tab).
constexpr int kBindingsPage = 3;

inline int TabStep(int page_count) {
    return page_count > 0 ? (kTabRight - kTabLeft) / page_count : 0;
}

// Word-wraps a row description to width. Explicit line breaks are kept when the text then fits in
// max_lines; otherwise the text reflows as one paragraph. More than max_lines means it does not fit.
std::vector<std::string> WrapHelp(std::string_view text, int width, int max_lines,
                                  const std::function<int(std::string_view)> &measure);
// Width of text in the game font at a PC page point size.
int TextWidth(std::string_view text, int size);

struct State {
    bool                       open = false;
    int                        page = 0;
    std::array<int, kMaxPages> row{};
    std::array<int, kMaxPages> first{};
    int                binding = -1;
    bool               save_failed = false;
};

enum class HitKind {
    None,
    Entry,
    Tab,
    Row,
    Previous,
    Next,
    Back,
    ResetBinding
};

struct Hit {
    HitKind kind = HitKind::None;
    int     index = -1;
    int     direction = 0;
};

struct Point {
    float x;
    float y;
};

Point PointerToUi(float x, float y, int window_width, int window_height, const gfx::LogicalMapping &mapping);
Point PointerPosition();
bool  Pointing();
// Handles PC options input and forwards retail Options cancel to the pad edge.
bool Run();
Hit  HitTest(const State &state, float x, float y);
void SelectPage(State &state, int page);
void MoveRow(State &state, int direction);
void NavigatePointer(State &state, Point point, bool moved, float wheel);

// Raw held-button repeat is independent of the retail menu's repeat bookkeeping.
class Repeat {
public:
    std::uint16_t Step(std::uint16_t held);
    void          Reset(std::uint16_t held = 0);

private:
    std::array<int, 16> frames_{};
};

extern State g_screen;
void         Draw();

} // namespace options
