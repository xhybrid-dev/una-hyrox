/**
 ******************************************************************************
 * @file    MapScreen.hpp
 * @brief   The stage as a small map: choose the way on, walk it, see what
 *          the step found.
 *
 * The map is one object that draws the whole stage in one pass (paths and
 * the places' icons, Paint.hpp), so it costs one widget, not one per place;
 * the runner's marker is a separate pair of dots, so it can blink and walk
 * without redrawing the map.
 *
 *   L1  the other way (where there are two)    R1  go
 *   L2  the pack                                R2 held: leave
 *
 * Under the chosen way's name, what lies that way (a spirit, a hazard, a
 * hut...), so a fork is a choice.
 *
 * After a step: the runner walks the line (about a second), then a duel or
 * an obstacle takes over, or a card says what was there (any button).
 ******************************************************************************
 */

#ifndef TRAILQUEST_MAP_SCREEN_HPP
#define TRAILQUEST_MAP_SCREEN_HPP

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Ui.hpp"

class MapScreen : public Screen
{
public:
    explicit MapScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onWorld() override;

protected:
    void build() override;
    void onKey(uint8_t code) override;
    void onFrame(uint32_t nowMs) override;

private:
    enum class Mode : uint8_t { Choose, Waiting, Walking, Card };

    static void drawCb(lv_event_t* e);
    void draw(lv_event_t* e);
    void refresh();
    void placeRunner(int32_t x, int32_t y);
    void arrive();
    void showCardFor(const TrailQuest::WorldView& w);

    lv_obj_t*   mMap     = nullptr;
    lv_obj_t*   mRing    = nullptr;
    lv_obj_t*   mDot     = nullptr;
    lv_obj_t*   mClock   = nullptr;
    lv_obj_t*   mExit    = nullptr;
    lv_obj_t*   mLeads   = nullptr;   ///< What lies the chosen way.
    Ui::Corners mCorners;
    Ui::Bar     mStamina;
    Ui::Card    mCard;
    char        mClockText[8] = {};
    Mode        mMode     = Mode::Choose;
    uint8_t     mChoice   = 0;      ///< Which exit L1 has picked.
    uint8_t     mWalkStep = 0;
    uint32_t    mLastBlink = 0;
    bool        mBlink    = true;

    /// The answer (World::ack) whose card was last shown, kept across
    /// screens, so coming back from the pack does not show it again.
    static uint16_t sCardAck;
};

#endif // TRAILQUEST_MAP_SCREEN_HPP
