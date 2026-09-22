/**
 ******************************************************************************
 * @file    AppMenu.hpp
 * @date    17-04-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Menu configuration.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef APP_MENU_HPP
#define APP_MENU_HPP

#include <cstdint>
#include "Settings.hpp"

// =============================================================================
// App::MenuNav  --  Navigation state with hierarchical position tracking.
//
// Usage in Model:
//   App::MenuNav::Nav mMenu {};
//   App::MenuNav::Nav& menu() { return mMenu; }
//
// Usage in Presenter:
//   activate()   -- view.setPositionId(model->menu().get());           // root level
//                  view.setPositionId(model->menu().intervals.get());  // sub-level
//                  model->menu().intervals.resetChildren();
//   deactivate() -- model->menu().set(view.getPositionId());
//                  model->menu().reset(); // full tree reset (e.g. on idle timeout)
// =============================================================================
namespace App::MenuNav
{

// -----------------------------------------------------------------------------
// Position<TMenu>
// Stores the selected index for one menu level.
// TMenu must expose ID_DEFAULT as an unscoped enum member.
// -----------------------------------------------------------------------------
template<typename TMenu>
struct Position {
    uint16_t value = static_cast<uint16_t>(TMenu::ID_DEFAULT);

    void     set(uint16_t v) { value = v; }
    uint16_t get() const     { return value; }
    void     reset()         { value = static_cast<uint16_t>(TMenu::ID_DEFAULT); }
};


// -----------------------------------------------------------------------------
// Menu descriptors
// Each struct describes one menu screen:
//   enum Id     -- ordered item list (ID_COUNT and ID_DEFAULT required)
//   kValues[]   -- optional: domain values mapped 1-to-1 to enum items
// -----------------------------------------------------------------------------

struct Root {
    enum Id { ID_START = 0, ID_FORMAT, ID_LAST_RACE, ID_SETTINGS,
              ID_COUNT, ID_DEFAULT = ID_START };

    /// Race format picker (brief 8.2 item 1).
    struct Format {
        enum Id { ID_FULL = 0, ID_HALF_A, ID_HALF_B,
                  ID_COUNT, ID_DEFAULT = ID_FULL };
    };

    /// Settings wheel (brief 8.2 item 2). Target finish is read-only and
    /// hidden until F14 ships, so it is not an item here.
    struct Settings {
        enum Id { ID_ROXZONE = 0, ID_RUN_DISTANCE, ID_LOCKOUT, ID_VIBRATE,
                  ID_COUNT, ID_DEFAULT = ID_ROXZONE };
    };
};


// RaceView is the in-race screen; its action menu is always an overlay of it.
struct RaceView {
    enum Id { ID_MAIN = 0, ID_SPLITS, ID_STATUS,
              ID_COUNT, ID_DEFAULT = ID_MAIN };

    /// Brief 8.2 item 5.
    struct Action {
        enum Id { ID_RESUME = 0, ID_UNDO_SPLIT, ID_PAUSE, ID_END, ID_DISCARD,
                  ID_COUNT, ID_DEFAULT = ID_RESUME };
    };
};


// -----------------------------------------------------------------------------
// Nav -- the whole tree, one position per level.
// -----------------------------------------------------------------------------
struct Nav : Position<Root> {

    struct RaceViewNav : Position<RaceView> {
        Position<RaceView::Action> action;

        void resetChildren() { action.reset(); }
        void reset()         { Position<RaceView>::reset(); resetChildren(); }
    };

    Position<Root::Format>   format;
    Position<Root::Settings> settings;
    RaceViewNav              race;

    void resetChildren() { format.reset(); settings.reset(); race.reset(); }
    void reset()         { Position<Root>::reset(); resetChildren(); }
};

} // namespace App::MenuNav

#endif // APP_MENU_HPP
