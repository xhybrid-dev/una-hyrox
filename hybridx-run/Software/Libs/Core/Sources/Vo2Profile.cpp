/**
 ******************************************************************************
 * @file    Vo2Profile.cpp
 * @brief   Age, max HR and resting HR for the VO2max estimate.
 ******************************************************************************
 */

#include "Vo2Profile.hpp"

#include "Vo2Config.hpp"

namespace RunVo2
{

namespace
{

// Civil date from days since 1970-01-01 (H. Hinnant's days_from_civil inverse).
void civilFromDays(int64_t z, int32_t& y, uint32_t& m)
{
    z += 719468;
    const int64_t  era = (z >= 0 ? z : z - 146096) / 146097;
    const uint32_t doe = static_cast<uint32_t>(z - era * 146097);
    const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const uint32_t mp  = (5 * doy + 2) / 153;
    m = mp < 10 ? mp + 3 : mp - 9;
    y = static_cast<int32_t>(yoe + era * 400 + (m <= 2 ? 1 : 0));
}

}  // namespace

uint8_t ageAt(uint16_t birthYear, uint8_t birthMonth, std::time_t nowUtc)
{
    if (birthYear == 0 || nowUtc <= 0) {
        return 0;
    }
    const uint8_t month = (birthMonth >= 1 && birthMonth <= 12) ? birthMonth : 1;

    int32_t  y = 0;
    uint32_t m = 0;
    civilFromDays(static_cast<int64_t>(nowUtc) / 86400, y, m);

    int32_t age = y - static_cast<int32_t>(birthYear);
    if (m < month) {
        --age;
    }
    if (age <= 0 || age > 120) {
        return 0;
    }
    return static_cast<uint8_t>(age);
}

uint8_t tanakaMaxHr(uint8_t ageYears)
{
    // 208 - 0.7 x age in tenths, rounded half up: (2080 - 7 x age + 5) / 10.
    const int32_t tenths = 2080 - 7 * static_cast<int32_t>(ageYears);
    return static_cast<uint8_t>((tenths + 5) / 10);
}

Profile resolveProfile(const ProfileInput& in, std::time_t nowUtc)
{
    Profile p;
    p.ageYears = ageAt(in.birthYear, in.birthMonth, nowUtc);

    if (in.enteredMaxHr != 0) {
        p.maxHr     = in.enteredMaxHr;
        p.maxSource = MaxHrSource::Entered;
    } else if (p.ageYears != 0) {
        p.maxHr     = tanakaMaxHr(p.ageYears);
        p.maxSource = MaxHrSource::Formula;
        if (in.autoMaxHr > p.maxHr && in.autoMaxHr <= Config::kMaxHrBpm) {
            p.maxHr     = in.autoMaxHr;
            p.maxSource = MaxHrSource::Observed;
        }
    } else {
        p.status = ProfileStatus::NeedsAge;
        return p;
    }

    auto plausibleRest = [](uint8_t bpm) {
        return bpm >= Config::kMinRestingHr && bpm <= Config::kMaxRestingHr;
    };
    if (plausibleRest(in.enteredRestHr)) {
        p.restHr = in.enteredRestHr;
    } else if (plausibleRest(in.watchRestHr)) {
        p.restHr = in.watchRestHr;
    } else {
        p.status = ProfileStatus::NeedsRestingHr;
        return p;
    }

    if (p.maxHr < p.restHr + Config::kMinHrReserve) {
        p.status = ProfileStatus::ReserveTooSmall;
        return p;
    }
    p.status = ProfileStatus::Ok;
    return p;
}

}  // namespace RunVo2
