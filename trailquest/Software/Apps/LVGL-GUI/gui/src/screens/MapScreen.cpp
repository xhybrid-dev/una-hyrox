/**
 ******************************************************************************
 * @file    MapScreen.cpp
 * @brief   The stage as a small map (see MapScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MapScreen.hpp"

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "Layout.hpp"
#include "Narrate.hpp"
#include "data/MapLayout.hpp"
#include "gui/Art.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/widgets/Paint.hpp"

namespace Draw  = SDK::LVGL::Draw;
namespace Btn   = SDK::GUI::Button;
namespace Label = TrailQuest::Narrate::Label;
using SDK::LVGL::Buttons;
using namespace TrailQuest;

uint16_t MapScreen::sCardAck = 0xFFFF;

namespace
{
constexpr uint8_t  kWalkFrames = 8;   ///< A step along the map, frames (0.8 s).
constexpr uint32_t kPath       = 0x808040;
constexpr uint32_t kChosen     = Ui::Ink::kAmber;

constexpr uint8_t bitOf(uint8_t stage) { return static_cast<uint8_t>(1u << (stage < 8 ? stage : 7)); }

/// One place's icon, centred at (x, y), in its 15 x 15 box.
void icon(const Paint::Canvas& c, NodeKind kind, int32_t x, int32_t y, bool spent)
{
    switch (kind) {
        case NodeKind::Start:
            Paint::rect(c, x - 4, y - 7, 2, 14, 0xC0C0C0);                       // pole
            Paint::tri(c, x - 2, y - 7, x + 6, y - 4, x - 2, y - 1, 0xC00000);    // flag
            break;
        case NodeKind::Trail:
            Paint::disc(c, x, y, 3, kPath);
            break;
        case NodeKind::Encounter:
            Paint::disc(c, x, y, 7, 0x000040);
            Paint::disc(c, x, y, 6, 0x800080);                                   // a spirit's mark
            Paint::rect(c, x - 1, y - 4, 3, 5, 0xC0C0C0);                        // "!"
            Paint::rect(c, x - 1, y + 3, 3, 2, 0xC0C0C0);
            break;
        case NodeKind::Obstacle:
            Paint::tri(c, x - 7, y + 6, x - 1, y - 6, x + 5, y + 6, 0x808080);    // rocks
            Paint::tri(c, x - 1, y + 6, x + 3, y - 1, x + 7, y + 6, 0x404040);
            break;
        case NodeKind::Hut:
            Paint::tri(c, x - 7, y, x, y - 7, x + 7, y, 0xC00000);               // roof
            Paint::rect(c, x - 5, y, 11, 7, 0xC08040);                           // walls
            Paint::rect(c, x - 1, y + 3, 3, 4, 0x400000);                        // door
            break;
        case NodeKind::Cache:
            Paint::rect(c, x - 6, y - 4, 13, 10, spent ? 0x404040 : 0xC08000);   // a box
            Paint::rect(c, x - 6, y - 1, 13, 2, spent ? 0x808080 : 0x804000);
            break;
        case NodeKind::Boss:
            Paint::disc(c, x, y, 8, spent ? 0x404040 : 0x800000);
            Paint::rect(c, x - 4, y - 3, 3, 3, spent ? 0x808080 : 0xC0C000);    // eyes
            Paint::rect(c, x + 2, y - 3, 3, 3, spent ? 0x808080 : 0xC0C000);
            Paint::rect(c, x - 3, y + 3, 7, 2, spent ? 0x808080 : 0xC0C0C0);    // teeth
            break;
    }
}
} // namespace

void MapScreen::build()
{
    mMap = Paint::surface(mRoot, 0, 0, 240, 240, &MapScreen::drawCb, this);
    mRing = Draw::dot(mRoot, 0, 0, 7, 0xC0C0C0);
    mDot  = Draw::dot(mRoot, 0, 0, 3, 0xC00000);   // the runner's red cap
    mClock = Ui::rowLabel(mRoot, Layout::kMapClockRow, mClockText);
    mCorners.createForMap(mRoot);
    mExit = Ui::rowLabel(mRoot, Layout::kExitRow, "", Ui::Ink::kAmber);
    mStamina.create(mRoot, 70, 198, 100, 6, Ui::Ink::kGreen);
    mCard.create(mRoot);
    refresh();
}

void MapScreen::onShow()
{
    const WorldView& w = mModel.world();
    const MapLayout::Point p = MapLayout::point(w.state.node);
    placeRunner(p.x, p.y);
    showCardFor(w);
}

void MapScreen::showCardFor(const WorldView& w)
{
    // Each answer's card is shown once: coming back from the pack does not
    // show it again.
    if (sCardAck == mModel.ack()) {
        return;
    }
    sCardAck = mModel.ack();
    TextFit::Lines lines;
    Narrate::arrival(lines, w);
    if (lines.count == 0) {
        return;
    }
    mCard.show(lines);
    mMode = Mode::Card;
    refresh();
}

void MapScreen::refresh()
{
    const WorldView& w    = mModel.world();
    const NodeDef&   here = nodeDef(w.state.stage, w.state.node);
    if (mChoice >= here.exitCount) {
        mChoice = 0;
    }
    Narrate::clock(mClockText, w.state.minutes);
    lv_label_set_text_static(mClock, mClockText);
    mStamina.set(w.state.stamina, runnerMaxWithPartners(w.state).stamina);

    const bool choosing = mMode == Mode::Choose && here.exitCount > 0;
    Ui::setText(mExit, choosing ? here.exits[mChoice].label : "", Ui::Ink::kAmber);
    if (mMode == Mode::Card) {
        mCorners.set(nullptr, Label::kOk, nullptr, nullptr);
        setArcs(Buttons::NONE, Buttons::NONE, Buttons::GREEN, Buttons::RED);
    } else if (choosing) {
        const bool two = here.exitCount > 1;
        mCorners.set(two ? Label::kPath : nullptr, Label::kGo, Label::kPack, nullptr);
        setArcs(two ? Buttons::AMBER : Buttons::NONE, Buttons::WHITE, Buttons::GREEN, Buttons::RED);
    } else {
        mCorners.set(nullptr, nullptr, nullptr, nullptr);
        setArcs(Buttons::NONE, Buttons::NONE, Buttons::NONE, Buttons::RED);
    }
    lv_obj_invalidate(mMap);
}

void MapScreen::placeRunner(int32_t x, int32_t y)
{
    lv_obj_set_pos(mRing, x - 7, y - 7);
    lv_obj_set_pos(mDot, x - 3, y - 3);
}

void MapScreen::onKey(uint8_t code)
{
    switch (mMode) {
        case Mode::Card:
            if (code == Btn::R1 || code == Btn::L1 || code == Btn::L2) {
                mCard.hide();
                mMode = Mode::Choose;
                refresh();
            }
            break;
        case Mode::Choose: {
            const NodeDef& here = nodeDef(mModel.world().state.stage, mModel.world().state.node);
            if (code == Btn::L1 && here.exitCount > 1) {
                mChoice = static_cast<uint8_t>((mChoice + 1u) % here.exitCount);
                refresh();
            } else if (code == Btn::R1 && here.exitCount > 0) {
                mMode = Mode::Waiting;
                mModel.travel(mChoice);
                refresh();
            } else if (code == Btn::L2) {
                ScreenManager::instance().goTo(ScreenId::Pack);
            }
        } break;
        default:
            break;
    }
}

void MapScreen::onWorld()
{
    if (mModel.waiting()) {
        return;
    }
    const WorldView& w = mModel.world();
    if (mMode == Mode::Waiting) {
        if (w.cause == ViewCause::Arrived) {
            mMode     = Mode::Walking;
            mWalkStep = 0;
            refresh();
            return;
        }
        mMode = Mode::Choose;   // refused: nothing moved
    }
    refresh();
}

void MapScreen::arrive()
{
    const WorldView& w = mModel.world();
    mChoice = 0;
    const ScreenId next = ScreenManager::screenFor(w);
    if (next != ScreenId::Map) {
        ScreenManager::instance().goTo(next);
        return;
    }
    mMode = Mode::Choose;
    showCardFor(w);
    refresh();
}

void MapScreen::onFrame(uint32_t nowMs)
{
    if (mMode == Mode::Walking) {
        const WorldView&       w    = mModel.world();
        const MapLayout::Point from = MapLayout::point(w.fromNode);
        const MapLayout::Point to   = MapLayout::point(w.state.node);
        ++mWalkStep;
        const int32_t t = mWalkStep > kWalkFrames ? kWalkFrames : mWalkStep;
        placeRunner(from.x + (to.x - from.x) * t / kWalkFrames, from.y + (to.y - from.y) * t / kWalkFrames);
        Draw::setHidden(mRing, false);
        Draw::setHidden(mDot, false);
        if (mWalkStep >= kWalkFrames) {
            arrive();
        }
        return;
    }
    if (nowMs - mLastBlink >= 500) {
        mLastBlink = nowMs;
        mBlink     = !mBlink;
        Draw::setHidden(mRing, !mBlink);
    }
}

void MapScreen::drawCb(lv_event_t* e)
{
    static_cast<MapScreen*>(lv_event_get_user_data(e))->draw(e);
}

void MapScreen::draw(lv_event_t* e)
{
    const Paint::Canvas c  = Paint::Canvas::of(e);
    const WorldView&    w  = mModel.world();
    const StageDef&     st = stageDef(w.state.stage);

    Paint::rect(c, 0, 0, 240, 240, Art::groundFor(w.state.stage, w.timeOfDay));

    // Paths first, the chosen one last so it lies on top.
    const NodeDef& here   = nodeDef(w.state.stage, w.state.node);
    const bool     choose = mMode == Mode::Choose && here.exitCount > 0;
    for (uint8_t n = 0; n < st.nodeCount && n < MapLayout::kPlaces; ++n) {
        const MapLayout::Point a = MapLayout::point(n);
        for (uint8_t i = 0; i < st.nodes[n].exitCount; ++i) {
            const MapLayout::Point b = MapLayout::point(st.nodes[n].exits[i].to);
            Paint::line(c, a.x, a.y, b.x, b.y, 4, kPath);
        }
    }
    if (choose) {
        const MapLayout::Point a = MapLayout::point(w.state.node);
        const MapLayout::Point b = MapLayout::point(here.exits[mChoice].to);
        Paint::line(c, a.x, a.y, b.x, b.y, 4, kChosen);
        Paint::disc(c, b.x, b.y, 11, kChosen);
    }
    for (uint8_t n = 0; n < st.nodeCount && n < MapLayout::kPlaces; ++n) {
        const MapLayout::Point p    = MapLayout::point(n);
        const NodeKind         kind = st.nodes[n].kind;
        const bool spent = (kind == NodeKind::Cache && (w.state.cachesTaken & bitOf(w.state.stage)) != 0)
                        || (kind == NodeKind::Boss && (w.state.bossesBeaten & bitOf(w.state.stage)) != 0);
        icon(c, kind, p.x, p.y, spent);
    }
}
