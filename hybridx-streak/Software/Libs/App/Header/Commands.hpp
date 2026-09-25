/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   HybridX Streak's Service <-> GUI message contract.
 *
 * IDs sit in the SDK's app-private range (MessageTypes.hpp); the kernel never
 * interprets them. Every message is checked against the kernel's 256-byte pool
 * block at compile time, because an oversized send fails silently on the
 * watch while working in the simulator (hybridx-race NOTES 0.9).
 *
 * The service sends a view whenever it changes, and the moments to play once.
 * The GUI queue is 10 deep and drops the oldest, so an update is at most six
 * messages (PLAN S2).
 ******************************************************************************
 */

#ifndef STREAK_COMMANDS_HPP
#define STREAK_COMMANDS_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include "Goal.hpp"
#include "StreakEvents.hpp"
#include "StreakTypes.hpp"
#include "StreakView.hpp"

#pragma pack(push, 4)

namespace CustomMessage
{

// -- Service -> GUI ------------------------------------------------------------
constexpr SDK::MessageType::Type HOME_VIEW = 0x00000001;
constexpr SDK::MessageType::Type MOMENTS   = 0x00000002;
constexpr SDK::MessageType::Type WEEK_LIST = 0x00000003;
constexpr SDK::MessageType::Type APP_NAMES = 0x00000004;
constexpr SDK::MessageType::Type TROPHIES  = 0x00000005;
constexpr SDK::MessageType::Type GOAL_VIEW = 0x00000006;

// -- GUI -> Service ------------------------------------------------------------
constexpr SDK::MessageType::Type CELEBRATE       = 0x00000080;
constexpr SDK::MessageType::Type LOG_MANUAL      = 0x00000081;
constexpr SDK::MessageType::Type WEEK_ACTION     = 0x00000082;
constexpr SDK::MessageType::Type SHIELD_DECISION = 0x00000083;
constexpr SDK::MessageType::Type SET_GOAL        = 0x00000084;

/// Everything the home screen shows.
struct HomeView : public SDK::MessageBase {
    Streak::HomeView view {};
    HomeView() : SDK::MessageBase(HOME_VIEW) {}
};

/// What happened since the GUI last heard: played once, in order (DESIGN 6).
struct Moments : public SDK::MessageBase {
    Streak::Events events {};
    Moments() : SDK::MessageBase(MOMENTS) {}
};

/// One line of "This week".
struct WeekItem {
    uint8_t  kind    = 0;      ///< Streak::Kind
    uint8_t  status  = 0;      ///< Streak::Status
    uint8_t  manual  = 0;      ///< 1 for a manual log (undo), 0 for a recorded one (exclude)
    uint8_t  app     = 0xFF;   ///< index into AppNames; 0xFE manual, 0xFF unknown
    uint16_t minutes = 0;
    uint8_t  weekday = 0;      ///< 0 = Sunday
    uint8_t  hour    = 0;
};

struct WeekList : public SDK::MessageBase {
    static constexpr uint8_t kMax = 16;
    WeekItem items[kMax] {};
    uint8_t  count      = 0;
    uint8_t  overflow   = 0;   ///< counted sessions beyond the list
    uint8_t  minMinutes = 0;   ///< for "6 min · under 10"
    uint8_t  scope      = Streak::kScopeAny;
    WeekList() : SDK::MessageBase(WEEK_LIST) {}
};

/// App folder names the week list refers to.
struct AppNames : public SDK::MessageBase {
    static constexpr uint8_t kMax   = 8;
    static constexpr uint8_t kChars = 16;
    char names[kMax][kChars] {};
    AppNames() : SDK::MessageBase(APP_NAMES) {}
};

/// The trophy case.
struct Trophies : public SDK::MessageBase {
    static constexpr uint8_t kRecent = 12;
    uint16_t weeksAchieved = 0;   ///< live: the summits climbed follow from it
    uint16_t lifetime      = 0;   ///< qualifying sessions, live
    uint16_t longest       = 0;   ///< longest streak, live
    uint8_t  bestWeek      = 0;
    uint8_t  badges        = 0;   ///< bit i = kSessionBadges[i]
    uint8_t  recent[kRecent] {};  ///< Streak::Outcome of the latest weeks, oldest first
    uint8_t  recentCount   = 0;
    Trophies() : SDK::MessageBase(TROPHIES) {}
};

/// The goal, and one waiting for next week.
struct GoalView : public SDK::MessageBase {
    Streak::Goal goal {};
    Streak::Goal pending {};
    uint8_t      hasPending = 0;
    GoalView() : SDK::MessageBase(GOAL_VIEW) {}
};

/// The moments the watch marks with a buzz. Only a service may drive the
/// vibration motor (no SDK GUI does), so the GUI asks.
enum class Moment : uint8_t {
    SessionFound,   ///< a new activity counted
    StepUp,         ///< this week's target reached: one step up the mountain
    Summit,         ///< a summit reached
    Shield,         ///< a shield spent to save the streak
};

struct Celebrate : public SDK::MessageBase {
    Moment moment = Moment::SessionFound;
    Celebrate() : SDK::MessageBase(CELEBRATE) {}
    explicit Celebrate(Moment m) : Celebrate() { moment = m; }
};

/// Log a session the watch did not record: today or yesterday (S7).
struct LogManual : public SDK::MessageBase {
    uint8_t kind      = 0;   ///< Streak::Kind
    uint8_t yesterday = 0;
    LogManual() : SDK::MessageBase(LOG_MANUAL) {}
    LogManual(uint8_t k, bool y) : LogManual() { kind = k; yesterday = y ? 1 : 0; }
};

/// Undo a manual log, or exclude / include a recorded session.
struct WeekAction : public SDK::MessageBase {
    enum : uint8_t { Undo, ToggleExclude };
    uint8_t action = Undo;
    uint8_t index  = 0;
    WeekAction() : SDK::MessageBase(WEEK_ACTION) {}
    WeekAction(uint8_t a, uint8_t i) : WeekAction() { action = a; index = i; }
};

/// The answer to a shield offer.
struct ShieldDecision : public SDK::MessageBase {
    uint8_t use = 0;
    ShieldDecision() : SDK::MessageBase(SHIELD_DECISION) {}
    explicit ShieldDecision(bool u) : ShieldDecision() { use = u ? 1 : 0; }
};

/// A new goal from the watch's Settings screen.
struct SetGoal : public SDK::MessageBase {
    Streak::Goal goal {};
    SetGoal() : SDK::MessageBase(SET_GOAL) {}
    explicit SetGoal(const Streak::Goal& g) : SetGoal() { goal = g; }
};

constexpr size_t kMaxMessageBytes = 256u;
#define STREAK_ASSERT_FITS_POOL(T) \
    static_assert(sizeof(T) <= kMaxMessageBytes, #T " exceeds the 256-byte kernel pool block")
STREAK_ASSERT_FITS_POOL(HomeView);
STREAK_ASSERT_FITS_POOL(Moments);
STREAK_ASSERT_FITS_POOL(WeekList);
STREAK_ASSERT_FITS_POOL(AppNames);
STREAK_ASSERT_FITS_POOL(Trophies);
STREAK_ASSERT_FITS_POOL(GoalView);
STREAK_ASSERT_FITS_POOL(Celebrate);
STREAK_ASSERT_FITS_POOL(LogManual);
STREAK_ASSERT_FITS_POOL(WeekAction);
STREAK_ASSERT_FITS_POOL(ShieldDecision);
STREAK_ASSERT_FITS_POOL(SetGoal);

} // namespace CustomMessage

#pragma pack(pop)

#endif // STREAK_COMMANDS_HPP
