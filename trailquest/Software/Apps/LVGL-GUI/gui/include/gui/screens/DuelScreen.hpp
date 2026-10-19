/**
 ******************************************************************************
 * @file    DuelScreen.hpp
 * @brief   A duel with a Trail Spirit, turn by turn.
 *
 * The spirit steps out (its name and what it does to a runner), then each turn:
 *
 *   L1  Surge   a hard push (costs stamina)     R1  Skill: the technique named
 *   L2  Settle  steady, and recover some        R2  Fuel: pick an item
 *
 * then the timing beat (any button strikes; if it is on), then the turn plays
 * back event by event: a line of text, the hit fighter flashes, the bars move,
 * the wrist buzzes. The first duel of a game explains the four moves first
 * (a card, three pages). On a turn the boss tells (its twist), a card warns
 * first and says what to do. At the end, what was won or lost, and on (a beaten boss
 * has its story page before the stage clear).
 *
 * The duel runs in the service; this screen sends the turn and plays back the
 * events the answer carries (DuelView::log), from the numbers it showed
 * before, so the bars move with the story and land on the service's numbers.
 ******************************************************************************
 */

#ifndef TRAILQUEST_DUEL_SCREEN_HPP
#define TRAILQUEST_DUEL_SCREEN_HPP

#include "Duel.hpp"
#include "gui/Sprite.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Ui.hpp"

class DuelScreen : public Screen
{
public:
    explicit DuelScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onWorld() override;

    /// A new game: the moves are explained again at its first duel.
    static void newGame() { sHelpSeen = false; }

protected:
    void build() override;
    void onKey(uint8_t code) override;
    void onFrame(uint32_t nowMs) override;
    bool rawKeys() const override { return mMode == Mode::Beat; }

private:
    enum class Mode : uint8_t { Intro, Help, Tell, Choose, Fuel, Beat, Waiting, Playback, End };

    void setMode(Mode m);
    void choose(TrailQuest::Action a);
    void send(TrailQuest::BeatResult beat);
    void showEvent(uint8_t i);
    void snapToView();
    void refreshNumbers();
    void say(const TrailQuest::TextFit::Lines& lines, uint32_t color);
    bool nextFuel(uint8_t from);
    void showFuel();
    void showHelp(uint8_t page);
    void toChoose();
    uint8_t helpPages() const;

    Sprite      mRunner;
    Sprite      mSpirit;
    Ui::Bar     mSpiritBar;
    Ui::Bar     mRunnerBar;
    lv_obj_t*   mSpiritNum = nullptr;
    lv_obj_t*   mRunnerNum = nullptr;
    lv_obj_t*   mNarr[2]   = {};
    Ui::Corners mCorners;
    Ui::BeatBar mBeat;
    Ui::Card    mCard;

    char                       mSpiritText[16] = {};
    char                       mRunnerText[16] = {};
    char                       mFuelText[16]   = {};
    TrailQuest::TextFit::Lines mSaid;

    Mode               mMode      = Mode::Intro;
    TrailQuest::Action mAction    = TrailQuest::Action::Surge;
    TrailQuest::ItemId mFuelItem  = TrailQuest::kNone;
    uint16_t           mShownRunner = 0;   ///< The numbers on screen, moved by the playback.
    uint16_t           mShownSpirit = 0;
    uint8_t            mRunnerFx  = 0;     ///< The effects on screen, likewise.
    uint8_t            mSpiritFx  = 0;
    uint8_t            mHold      = 0;     ///< Frames the beat's verdict stays before the playback.
    bool               mStarted   = false; ///< The playback has shown its first event.
    uint8_t            mEvent     = 0;
    uint8_t            mEventFrame = 0;
    uint32_t           mLastFlip  = 0;
    uint8_t            mFrame     = 0;
    lv_obj_t*          mFlash     = nullptr;   ///< The sprite hit by the current event.
    uint8_t            mHelpPage  = 0;
    uint8_t            mTellTurn  = 0xFF;   ///< The turn whose boss warning has been shown.

    /// The help card (what each button does) is shown once a game, at the
    /// first duel's first turn.
    static bool sHelpSeen;
};

#endif // TRAILQUEST_DUEL_SCREEN_HPP
