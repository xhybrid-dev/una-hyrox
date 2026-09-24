/**
 ******************************************************************************
 * @file    Classifier.cpp
 * @brief   Sport and app folder to session kind (see the header).
 ******************************************************************************
 */

#include "Classifier.hpp"

#include <cstring>

namespace Streak
{

namespace
{
// una-sdk FitProfile.hpp, enum class Sport.
constexpr uint8_t kGeneric  = 0;
constexpr uint8_t kRunning  = 1;
constexpr uint8_t kCycling  = 2;
constexpr uint8_t kTraining = 10;
constexpr uint8_t kWalking  = 11;
constexpr uint8_t kHiking   = 17;

bool is(const char* folder, const char* name)
{
    return folder != nullptr && std::strcmp(folder, name) == 0;
}
} // namespace

Kind classify(uint8_t sport, uint8_t subSport, const char* appFolder)
{
    (void)subSport;   // every Running sub-sport (Treadmill included) is a run

    // Known apps first: the folder knows more than a Generic sport does.
    if (is(appFolder, "HybridXRace")) {
        return Kind::Hybrid;
    }
    switch (sport) {
        case kRunning:  return Kind::Run;
        case kCycling:  return Kind::Ride;
        case kWalking:
        case kHiking:   return Kind::Walk;
        case kTraining: return Kind::Strength;
        case kGeneric:  return is(appFolder, "Workout") ? Kind::Workout : Kind::Other;
        default:        return Kind::Other;   // outside the SDK's enum
    }
}

} // namespace Streak
