/**
 ******************************************************************************
 * @file    TextStack.hpp
 * @brief   X Training: a few centred lines of text, stacked down the round face.
 *
 * The plan's screens (Today, a day of the week, Saved) are lines of text in
 * different faces. The stack centres them as a block, each line on its own
 * centre, and fits each to the width the circle allows at its height: in its
 * own face if it can, else in the smaller face given, else cut with an
 * ellipsis (which the text-fit audit reports). A line of nullptr or "" is
 * left out and takes no room.
 ******************************************************************************
 */

#ifndef TEXT_STACK_HPP
#define TEXT_STACK_HPP

#include <cmath>
#include <cstdint>

#include "gui/Audit.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/widgets/WorkoutLabel.hpp"

class TextStack
{
public:
    static constexpr uint8_t kMaxLines = 7;

    struct Line {
        const char* text    = nullptr;
        Theme::Font font    = Theme::Font::Medium18;
        Theme::Font smaller = Theme::Font::Medium18;   ///< tried when the text is too wide
        uint32_t    color   = SDK::GUI::Color::WHITE;
    };

    /// Create the labels on @p parent (a container at the screen's origin).
    void init(lv_obj_t* parent)
    {
        for (WorkoutLabel& l : mLabels) {
            l.init(parent, Theme::Font::Medium18, SDK::GUI::Color::WHITE);
            l.setVisible(false);
            Audit::track(l.obj());
        }
    }

    /// Show @p n lines (at most kMaxLines), centred as a block on @p centreY. True if every
    /// line fitted whole (none cut with an ellipsis).
    bool set(const Line* lines, uint8_t n, int32_t centreY = 124)
    {
        bool whole = true;
        if (n > kMaxLines) {
            n = kMaxLines;
        }
        int32_t height = 0;
        for (uint8_t i = 0; i < n; ++i) {
            if (shown(lines[i])) {
                height += pitch(lines[i].font);
            }
        }
        int32_t y = centreY - height / 2;
        uint8_t used = 0;
        for (uint8_t i = 0; i < n; ++i) {
            const Line& line = lines[i];
            if (!shown(line)) {
                continue;
            }
            const int32_t p = pitch(line.font);
            WorkoutLabel& l = mLabels[used++];
            l.setFont(line.font);
            l.setColor(line.color);
            l.setAnchor(120, y + p / 2);
            l.setText(line.text);
            if (l.width() > clearWidth(l, y + p / 2) && line.smaller != line.font) {
                l.setFont(line.smaller);
            }
            whole = whole && l.width() <= clearWidth(l, y + p / 2);
            l.fit(clearWidth(l, y + p / 2));
            l.setVisible(mVisible);
            y += p;
        }
        mUsed = used;
        for (uint8_t i = used; i < kMaxLines; ++i) {
            mLabels[i].setText("");
            mLabels[i].setVisible(false);
        }
        return whole;
    }

    void setVisible(bool visible)
    {
        mVisible = visible;
        for (uint8_t i = 0; i < kMaxLines; ++i) {
            mLabels[i].setVisible(visible && i < mUsed);
        }
    }

    /// The labels, for the text-fit audit.
    WorkoutLabel& label(uint8_t i) { return mLabels[i < kMaxLines ? i : 0]; }

    /// The room a line in @p font takes, in pixels.
    static int32_t pitch(Theme::Font font)
    {
        switch (font) {
            case Theme::Font::Regular14:  return 20;
            case Theme::Font::Regular16:  return 22;
            case Theme::Font::SemiBold20:
            case Theme::Font::Italic20:   return 26;
            case Theme::Font::Medium25:
            case Theme::Font::SemiBold25: return 30;
            case Theme::Font::SemiBold30: return 36;
            case Theme::Font::SemiBold35: return 40;
            default:                      return 24;   // the 18s
        }
    }

    /// The width a line on @p l's font, centred on @p cy, may take and keep every corner of
    /// its text within kClear of the face's centre: clear of the buttons' arcs (whose inner
    /// edge is at 110; the text-fit audit's rule).
    static int32_t clearWidth(const WorkoutLabel& l, int32_t cy)
    {
        constexpr int32_t kClear = 108;
        const int32_t h  = lv_font_get_line_height(l.font());
        const int32_t up = cy - h / 2 < 120 ? 120 - (cy - h / 2) : 0;
        const int32_t dn = cy + h / 2 > 120 ? (cy + h / 2) - 120 : 0;
        const int32_t dy = up > dn ? up : dn;
        if (dy >= kClear) {
            return 0;
        }
        return 2 * static_cast<int32_t>(std::sqrt(static_cast<float>(kClear * kClear - dy * dy)));
    }

private:
    static bool shown(const Line& line) { return line.text != nullptr && line.text[0] != '\0'; }

    WorkoutLabel mLabels[kMaxLines];
    uint8_t      mUsed    = 0;
    bool         mVisible = true;
};

#endif // TEXT_STACK_HPP
