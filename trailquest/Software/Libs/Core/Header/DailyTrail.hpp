/**
 ******************************************************************************
 * @file    DailyTrail.hpp
 * @brief   The Daily Trail's seed: the same for everyone on the same date.
 *
 * Header-only. The Daily Trail (phase Q5) builds one stage from this seed, so
 * every player meets the same trail on the same day. Q1 fixes only the seed.
 ******************************************************************************
 */

#ifndef TRAILQUEST_DAILY_TRAIL_HPP
#define TRAILQUEST_DAILY_TRAIL_HPP

#include <cstdint>

#include "Rng.hpp"

namespace TrailQuest
{

/// A seed from a calendar date (year, month 1-12, day 1-31). Different dates
/// give different seeds; the salt keeps it apart from other uses of mixSeed.
inline uint32_t dailySeed(uint16_t year, uint8_t month, uint8_t day)
{
    const uint32_t ymd = static_cast<uint32_t>(year) * 10000u + static_cast<uint32_t>(month) * 100u + day;
    const uint32_t seed = mixSeed(ymd ^ 0x54515444u);   // "TQTD"
    return seed != 0u ? seed : 1u;
}

} // namespace TrailQuest

#endif // TRAILQUEST_DAILY_TRAIL_HPP
