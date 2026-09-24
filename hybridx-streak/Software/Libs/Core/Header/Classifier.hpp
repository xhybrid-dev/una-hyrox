/**
 ******************************************************************************
 * @file    Classifier.hpp
 * @brief   What kind of session a recorded activity was (PLAN 5.3).
 *
 * The FIT sport decides, except where the app folder says more: HybridX Race
 * writes Running/Generic (Race NOTES D2) but a race or sim is a Hybrid
 * session, and the SDK's Workout and HRMonitor apps both write Generic.
 * Sport values are FitProfile.hpp's Sport enum.
 ******************************************************************************
 */

#ifndef STREAK_CLASSIFIER_HPP
#define STREAK_CLASSIFIER_HPP

#include <cstdint>

#include "StreakTypes.hpp"

namespace Streak
{

Kind classify(uint8_t sport, uint8_t subSport, const char* appFolder);

} // namespace Streak

#endif // STREAK_CLASSIFIER_HPP
