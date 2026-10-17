/**
 ******************************************************************************
 * @file    Screens.hpp
 * @brief   X Weigh-in's screens, their base class and the switcher.
 *
 * Screen and ScreenManager follow the SDK's RunLVGL example
 * (Examples/Apps/RunLVGL/Software/Apps/LVGL-GUI/gui/.../screens/): a screen
 * owns one LVGL screen object and gets the kernel's button codes through
 * LV_EVENT_KEY; a switch is requested with goTo() and done on the next LVGL
 * pass, building the new screen before freeing the old, so the requesting
 * screen is never deleted under its own feet.
 *
 * The buttons (as labelled on the watch): L1 UP, L2 DOWN, R1 SELECT, R2 BACK.
 * On the four pages UP and DOWN page through Home, Graph, Recent and Body;
 * SELECT weighs in; BACK leaves the app.
 ******************************************************************************
 */

#ifndef SCREENS_HPP
#define SCREENS_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "GraphScale.hpp"
#include "WeighEntry.hpp"
#include "Window.hpp"
#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

enum class ScreenId : uint8_t {
    Home = 0,   // the four pages, in paging order
    Graph,
    Recent,
    Body,
    WeighIn,
    Saved,
};

/// Pages UP and DOWN cycle through (Home .. Body).
constexpr uint8_t kPageCount = 4;

class Screen : public ModelListener
{
public:
    explicit Screen(Model& model);
    virtual ~Screen();

    Screen(const Screen&)            = delete;
    Screen& operator=(const Screen&) = delete;

    /// Create the LVGL screen object and its widgets (calls build()).
    void create();

    /// The LVGL screen object, or nullptr before create().
    lv_obj_t* root() const { return mRoot; }

    /// Called after this screen has become the active one and the Model is bound.
    virtual void onShow() {}

    /// Called with an SDK::GUI::Button code (click, press or release).
    virtual void onKey(uint8_t code) { (void)code; }

protected:
    /// Populate mRoot with widgets.
    virtual void build() = 0;

    Model&    mModel;
    lv_obj_t* mRoot = nullptr;
    std::unique_ptr<SDK::LVGL::Buttons> mButtons;

private:
    static void keyEventCb(lv_event_t* e);
};

class ScreenManager
{
public:
    static ScreenManager& instance();

    /// Bind the model and show the first screen.
    void start(Model& model, ScreenId first);

    /// Request a switch; performed on the next frame.
    void goTo(ScreenId id);

private:
    ScreenManager() = default;

    static void asyncCb(void* user);
    void switchNow(ScreenId id);
    Screen* create(ScreenId id);

    Model*   mModel      = nullptr;
    Screen*  mCurrent    = nullptr;
    ScreenId mPending    = ScreenId::Home;
    bool     mPendingSet = false;
};

/// The frame the four pages share: a title, the page dots, the button marks,
/// and the keys (UP/DOWN page, SELECT weighs in, BACK exits).
class PageScreen : public Screen
{
public:
    PageScreen(Model& model, uint8_t page) : Screen(model), mPage(page) {}

    void onKey(uint8_t code) override;

protected:
    /// Make the title (empty title: none) and the page dots.
    void buildFrame(const char* title);

    uint8_t mPage;
};

class HomeScreen : public PageScreen
{
public:
    explicit HomeScreen(Model& model) : PageScreen(model, 0) {}
    void onShow() override { refresh(); }
    void onHome() override { refresh(); }

protected:
    void build() override;

private:
    void refresh();

    lv_obj_t* mCaption = nullptr;
    lv_obj_t* mTrend   = nullptr;
    lv_obj_t* mChange  = nullptr;
    lv_obj_t* mLast    = nullptr;
};

class GraphScreen : public PageScreen
{
public:
    explicit GraphScreen(Model& model) : PageScreen(model, 1) {}
    void onShow() override { refresh(); }
    void onGraph() override { refresh(); }

protected:
    void build() override;

private:
    void refresh();

    // Clear of the UP and DOWN marks at the left edge, which the scale labels
    // sit just right of, and of SELECT and BACK at the right.
    static constexpr int32_t kLeft   = 70;
    static constexpr int32_t kTop    = 60;
    static constexpr int32_t kWidth  = 140;
    static constexpr int32_t kHeight = 100;

    lv_obj_t*          mLine = nullptr;
    lv_obj_t*          mDots[XWeighIn::kWindowDays] = {};
    lv_obj_t*          mTopLabel = nullptr;
    lv_obj_t*          mBottomLabel = nullptr;
    lv_obj_t*          mSpan = nullptr;
    lv_obj_t*          mEmpty = nullptr;
    lv_point_precise_t mPoints[XWeighIn::kWindowDays] = {};
};

class RecentScreen : public PageScreen
{
public:
    explicit RecentScreen(Model& model) : PageScreen(model, 2) {}
    void onShow() override { refresh(); }
    void onGraph() override { refresh(); }

protected:
    void build() override;

private:
    void refresh();

    static constexpr uint8_t kRows = 5;
    lv_obj_t* mDates[kRows]   = {};
    lv_obj_t* mWeights[kRows] = {};
    lv_obj_t* mEmpty = nullptr;
};

class BodyScreen : public PageScreen
{
public:
    explicit BodyScreen(Model& model) : PageScreen(model, 3) {}
    void onShow() override { refresh(); }
    void onHome() override { refresh(); }

protected:
    void build() override;

private:
    void refresh();

    lv_obj_t* mBmi    = nullptr;
    lv_obj_t* mBmr    = nullptr;
    lv_obj_t* mHeight = nullptr;
    lv_obj_t* mAge    = nullptr;
    lv_obj_t* mNote   = nullptr;
};

class WeighInScreen : public Screen
{
public:
    explicit WeighInScreen(Model& model) : Screen(model) {}
    ~WeighInScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onSaveResult() override;

protected:
    void build() override;

private:
    static void tickCb(lv_timer_t* t);
    void step(int32_t steps);
    void show();

    lv_obj_t*            mTitle = nullptr;
    lv_obj_t*            mValue = nullptr;
    lv_obj_t*            mUnit  = nullptr;
    lv_obj_t*            mHint  = nullptr;
    lv_timer_t*          mTimer = nullptr;
    XWeighIn::HoldRepeat mHold;
    int32_t              mHeldDir = 0;   ///< +1 UP, -1 DOWN, 0 none
    int32_t              mValueH  = 0;   ///< 0.01 kg
    bool                 mSaving  = false;
};

class SavedScreen : public Screen
{
public:
    explicit SavedScreen(Model& model) : Screen(model) {}
    ~SavedScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onHome() override { refresh(); }

protected:
    void build() override;

private:
    static void doneCb(lv_timer_t* t);
    void refresh();

    lv_obj_t*   mHeading = nullptr;
    lv_obj_t*   mValue   = nullptr;
    lv_obj_t*   mLine1   = nullptr;
    lv_obj_t*   mLine2   = nullptr;
    lv_timer_t* mDone    = nullptr;
};

#endif // SCREENS_HPP
