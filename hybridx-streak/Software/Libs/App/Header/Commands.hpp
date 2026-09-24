/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   HybridX Streak's Service <-> GUI message contract.
 *
 * IDs sit in the SDK's app-private range (MessageTypes.hpp); the kernel never
 * interprets them. Every message is checked against the kernel's 256-byte pool
 * block at compile time, because an oversized send fails silently on the
 * watch while working in the simulator (hybridx-race NOTES 0.9).
 ******************************************************************************
 */

#ifndef STREAK_COMMANDS_HPP
#define STREAK_COMMANDS_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include "StreakView.hpp"

#pragma pack(push, 4)

namespace CustomMessage
{

// -- Service -> GUI ------------------------------------------------------------
constexpr SDK::MessageType::Type HOME_VIEW = 0x00000001;

// -- GUI -> Service ------------------------------------------------------------
constexpr SDK::MessageType::Type CELEBRATE = 0x00000080;

/// Everything the home screen shows (from phase S2; the S0 demo fabricates it).
struct HomeView : public SDK::MessageBase {
    Streak::HomeView view {};
    HomeView() : SDK::MessageBase(HOME_VIEW) {}
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

constexpr size_t kMaxMessageBytes = 256u;
#define STREAK_ASSERT_FITS_POOL(T) \
    static_assert(sizeof(T) <= kMaxMessageBytes, #T " exceeds the 256-byte kernel pool block")
STREAK_ASSERT_FITS_POOL(HomeView);
STREAK_ASSERT_FITS_POOL(Celebrate);

} // namespace CustomMessage

#pragma pack(pop)

#endif // STREAK_COMMANDS_HPP
