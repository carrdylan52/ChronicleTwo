#include <algorithm>
#include <string>
#include <string_view>

#include "font.hpp"
#include "mainloop.hpp"
#include "menudraw.hpp"
#include "options/rows.hpp"
#include "options/screen.hpp"
#include "platform/display.hpp"

namespace options {
namespace {

CFont Font(int size) {
    CFont font;
    font.SetDrawSize(size, size + 4);
    font.SetClearance(size, size + 5);
    font.SetFuchi(FUCHI_SHADOW_BLACK);
    return font;
}

void Text(std::string text, int x, int y, int width, bool gold = false, int size = 16) {
    CFont font = Font(size);
    font.SetColor(gold ? 128 : 118, gold ? 108 : 118, gold ? 64 : 118, 128);
    int w = 0, h = 0;
    font.CalcDrawWH(text.data(), &w, &h);
    // Keep both custom values and lists of key bindings inside their own column.
    if (w > width) {
        while (text.size() > 3 && w > width) {
            text.pop_back();
            std::string shortened = text + "...";
            font.CalcDrawWH(shortened.data(), &w, &h);
        }
        text += "...";
    }
    font.DrawDirect(text.data(), x, y);
}

void CenteredText(const std::string &text, int x, int y, int width, bool gold, int size) {
    Text(text, x + std::max(0, (width - TextWidth(text, size)) / 2), y, width, gold, size);
}

void Fill(int x, int y, int w, int h, int r, int g, int b, int alpha = 0xFF) {
    DrawMenuFillBox(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h),
                    alpha, r, g, b);
}

// The retail Options buttons' colours: lit for the active choice, dark for the others.
void Plate(int x, int y, int width, int height, bool lit) {
    Fill(x, y, width, height, lit ? 124 : 70, lit ? 96 : 52, lit ? 66 : 34);
    Fill(x + 1, y + 1, width - 2, height - 2, lit ? 93 : 50, lit ? 67 : 36, lit ? 45 : 21);
}

// A flat label like the retail page's own caption plates; it is not a button.
void Caption(int x, int y, int width, int height) { Fill(x, y, width, height, 36, 27, 20); }

void Help(const std::string &help) {
    const auto lines = WrapHelp(help, kHelpWidth, kHelpLines, [](std::string_view text) { return TextWidth(text, kHelpSize); });
    for (int line = 0; line < std::min<int>(kHelpLines, static_cast<int>(lines.size())); ++line) {
        std::string text = lines[line];
        // Text that cannot fit ends in an ellipsis on the last line.
        if (line == kHelpLines - 1 && static_cast<int>(lines.size()) > kHelpLines) {
            text += " " + lines[line + 1];
        }
        Text(text, kHelpX, 392 + line * 24, kHelpWidth, false, kHelpSize);
    }
}

void Tabs(const Hit &hit) {
    const int count = static_cast<int>(Pages().size());
    const int step = TabStep(count);
    for (int p = 0; p < count; ++p) {
        const int  x = kTabLeft + p * step;
        const int  width = (p == count - 1 ? kTabRight - x : step - 3);
        const bool selected = p == g_screen.page;
        Plate(x, kTabTop, width, kTabHeight, selected || (hit.kind == HitKind::Tab && hit.index == p));
        CenteredText(Pages()[p].name, x + 2, kTabTop + 2, width - 4, selected, 14);
    }
}

// Drawn like the retail list's bar: a dark track, a bevelled bronze thumb and its shadow.
void ScrollBar(int count, int first) {
    Fill(kScrollX, kScrollTop, 7, kScrollHeight, 48, 44, 41);
    const int shown = std::min(kVisibleRows, count);
    if (count <= shown) {
        return;
    }
    const int h = std::max(8, kScrollHeight * shown / count);
    const int y = kScrollTop + (kScrollHeight - h - 1) * first / (count - shown);
    Fill(kScrollX, y, 1, h, 160, 113, 67);
    Fill(kScrollX + 1, y, 1, h, 125, 88, 53);
    Fill(kScrollX + 2, y, 2, h, 99, 71, 47);
    Fill(kScrollX + 4, y, 1, h, 78, 56, 29);
    Fill(kScrollX + 5, y, 2, h, 17, 11, 8);
    Fill(kScrollX, y + h, 7, 1, 0, 0, 0);
}

void Rows(const Config &config) {
    const auto &page = Pages()[g_screen.page];
    const int   count = static_cast<int>(page.rows.size());
    const int   first = g_screen.first[g_screen.page];
    for (int row = first; row < std::min(count, first + kVisibleRows); ++row) {
        const int   y = kRowY + (row - first) * kRowStep;
        const auto &item = page.rows[row];
        const bool  selected = g_screen.row[g_screen.page] == row;
        if (selected) {
            Fill(kLeft, y + 1, kRight - kLeft, kRowStep - 2, 62, 50, 38);
            Text(">", kLeft + 1, y + 2, 12, true, 14);
        }
        Text(item.label, kLeft + 9, y + 2, kValueX - kLeft - 15, false, 15);
        Plate(kValueX, y + 2, kRight - kValueX, kRowStep - 4, false);
        const std::string value = g_screen.binding == row ? "Press a key..." : item.value(config);
        CenteredText(value, kValueX + 14, y + 3, kRight - kValueX - 28, selected, 14);
        if (selected && item.step) {
            Text("<", kValueX + 3, y + 3, 10, true, 14);
            Text(">", kRight - 11, y + 3, 10, true, 14);
        }
    }
    ScrollBar(count, first);
}

std::string Shortcuts(const Config &config) {
    if (g_screen.binding >= 0) {
        return "Escape: Cancel capture";
    }
    const auto confirm = LanguageCode > 0 ? "cross" : "circle";
    const auto cancel = LanguageCode > 0 ? "circle" : "cross";
    return ButtonName("l1", config) + "/" + ButtonName("r1", config) + ": Page   " + ButtonName(confirm, config) +
           ": Change   " + ButtonName(cancel, config) + ": Back";
}

} // namespace

int TextWidth(std::string_view text, int size) {
    CFont       font = Font(size);
    std::string copy(text);
    int         w = 0, h = 0;
    font.CalcDrawWH(copy.data(), &w, &h);
    return w;
}

void Draw() {
    // Retail owns the textured background, gold frame, title and transitions. Cover only its
    // contents while on the PC page; no form or message state is rewritten.
    ResetMenuScissor();
    const auto &config = ConfigGet();
    const auto  point = PointerPosition();
    const auto  hit = Pointing() ? HitTest(g_screen, point.x, point.y) : Hit{};
    if (!g_screen.open) {
        Plate(284, 51, 160, 25, hit.kind == HitKind::Entry);
        Text(ButtonName("select", config) + ": PC SETTINGS", 291, 54, 147, true, 13);
        return;
    }
    // Retail FormDraw paints the frame, its list and help text first. Repaint the frame's interior
    // in its own colour, so the PC rows sit in the retail frame with nothing above it.
    Fill(kPanelLeft, kPanelTop, kPanelRight - kPanelLeft, kPanelBottom - kPanelTop, 40, 36, 35);
    // The page's name takes the place of the plate that opened it.
    Caption(284, 51, 160, 25);
    CenteredText("PC SETTINGS", 286, 54, 156, true, 15);
    Tabs(hit);
    Rows(config);
    Plate(kBackLeft, kButtonTop, kBackWidth, kButtonHeight, hit.kind == HitKind::Back);
    CenteredText(g_screen.binding >= 0 ? "Cancel capture" : "Back to Options", kBackLeft + 3, kButtonTop + 6,
                 kBackWidth - 6, false, 12);
    if (g_screen.page == kBindingsPage && g_screen.binding < 0) {
        // The binding rows' help names their controls; this plate takes the shortcut line's place.
        Plate(kResetLeft, kButtonTop, kPanelRight - kResetLeft, kButtonHeight, hit.kind == HitKind::ResetBinding);
        CenteredText(ButtonName("square", config) + ": Restore binding", kResetLeft + 3, kButtonTop + 6,
                     kPanelRight - kResetLeft - 6, false, 12);
    } else {
        Caption(kHintLeft, kButtonTop, kPanelRight - kHintLeft, kButtonHeight);
        CenteredText(Shortcuts(config), kHintLeft + 3, kButtonTop + 6, kPanelRight - kHintLeft - 6, true, 12);
    }
    Fill(48, 386, 420, 60, 36, 24, 16);
    if (g_screen.save_failed) {
        Help("Could not save config.json. Settings work this session.\nCheck the save folder; change a setting to retry.");
    } else if (DisplayGetWarning() != DisplayWarning::None) {
        Help("The display could not apply that mode.\nThe setting shows the mode the window kept.");
    } else if (g_screen.binding >= 0) {
        Help("Press a key or mouse button. Escape cancels.\nEscape is reserved and cannot be bound.");
    } else {
        Help(Pages()[g_screen.page].rows[g_screen.row[g_screen.page]].help);
    }
}

} // namespace options
