/**
 ******************************************************************************
 * @file    StoryScreen.hpp
 * @brief   A few pages of story at the key moments (data/Story.hpp): the
 *          opening tale, a stage starting, its boss ahead, its boss beaten.
 *
 * Each page: a headline, the words on the card, and a strip of scene under
 * it with the runner and whoever the page is about. Then the screen the game
 * is at (ScreenManager::screenFor): the story sends no command, so the world
 * it was asked over is still the world when it ends.
 *
 *   L1  skip the rest                           R1  next page (OK on the last)
 *                                               R2 held: leave
 ******************************************************************************
 */

#ifndef TRAILQUEST_STORY_SCREEN_HPP
#define TRAILQUEST_STORY_SCREEN_HPP

#include "data/Story.hpp"
#include "gui/Sprite.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Ui.hpp"

class StoryScreen : public Screen
{
public:
    explicit StoryScreen(Model& model) : Screen(model) {}

    /// Show @p plan's pages now (switches to this screen).
    static void play(const TrailQuest::StoryPlan& plan);

    /// The boss-ahead page is told once a game (not again on a retry): true,
    /// and remembered, the first time it is asked for @p stage.
    static bool firstSightOfBoss(uint8_t stage);

    /// A new game: every boss is new again.
    static void newGame() { sBossSeen = 0; }

    void onShow() override;

protected:
    void build() override;
    void onKey(uint8_t code) override;
    void onFrame(uint32_t nowMs) override;

private:
    void showPage();
    void finish();

    lv_obj_t*   mHead = nullptr;
    lv_obj_t*   mSky  = nullptr;
    lv_obj_t*   mGround = nullptr;
    Ui::Card    mCard;
    Ui::Corners mCorners;
    Sprite      mRunner;
    Sprite      mWho;
    uint8_t     mPage     = 0;
    bool        mDone     = false;
    uint32_t    mLastFlip = 0;
    uint8_t     mFrame    = 0;

    /// What play() asked for, kept across the switch to this screen.
    static TrailQuest::StoryPlan sPlan;
    static uint8_t               sBossSeen;   ///< A bit a stage.
};

#endif // TRAILQUEST_STORY_SCREEN_HPP
