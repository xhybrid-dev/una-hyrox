/**
 ******************************************************************************
 * @file    ObstacleScreen.hpp
 * @brief   An obstacle on the trail: what it tests and the odds, the timing
 *          beat (if it is on), then whether the runner got over it.
 *
 *   R1  go (then any button strikes the beat), and OK on the result
 *   R2 held: leave (the obstacle is there again next time)
 ******************************************************************************
 */

#ifndef TRAILQUEST_OBSTACLE_SCREEN_HPP
#define TRAILQUEST_OBSTACLE_SCREEN_HPP

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Ui.hpp"

class ObstacleScreen : public Screen
{
public:
    explicit ObstacleScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onWorld() override;

protected:
    void build() override;
    void onKey(uint8_t code) override;
    void onFrame(uint32_t nowMs) override;
    bool rawKeys() const override { return mMode == Mode::Beat; }

private:
    enum class Mode : uint8_t { Intro, Beat, Waiting, Result };

    void attempt(TrailQuest::BeatResult beat);
    void setMode(Mode m);

    Ui::Card    mCard;
    Ui::Corners mCorners;
    Ui::BeatBar mBeat;
    lv_obj_t*   mHead = nullptr;
    Mode        mMode = Mode::Intro;
};

#endif // TRAILQUEST_OBSTACLE_SCREEN_HPP
