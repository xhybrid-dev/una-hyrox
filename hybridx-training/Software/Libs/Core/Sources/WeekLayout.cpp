/**
 ******************************************************************************
 * @file    WeekLayout.cpp
 * @brief   Placing a week's slots on the runner's days. See WeekLayout.hpp.
 ******************************************************************************
 */

#include "WeekLayout.hpp"

namespace XTraining {
namespace WeekLayout {

namespace {

/// Days after the long-run day, 0..6.
uint8_t after(uint8_t day, uint8_t longDay)
{
    return static_cast<uint8_t>((day + 7u - longDay) % 7u);
}

/// May @p day hold quality? Never L itself or the day after it; the day before
/// it only if @p allowBefore.
bool qualityAllowed(uint8_t day, uint8_t longDay, bool allowBefore)
{
    const uint8_t a = after(day, longDay);
    if (a == 0 || a == 1) {
        return false;
    }
    return a != 6 || allowBefore;
}

/// The smallest gap, round the week, between L and quality days at @p a1 and
/// @p a2 days after it (a1 < a2).
uint8_t smallestGap(uint8_t a1, uint8_t a2)
{
    const uint8_t g1 = a1;
    const uint8_t g2 = static_cast<uint8_t>(a2 - a1);
    const uint8_t g3 = static_cast<uint8_t>(7 - a2);
    uint8_t g = g1 < g2 ? g1 : g2;
    return g < g3 ? g : g3;
}

}  // namespace

bool valid(DayMask days, uint8_t longDay)
{
    const uint8_t n = dayCount(days);
    return longDay < 7 && ((days >> longDay) & 1u) && n >= kMinDays && n <= kMaxDays;
}

Layout place(DayMask days, uint8_t longDay, bool allowSeven)
{
    Layout l;
    days &= 0x7Fu;
    const uint8_t n = dayCount(days);
    const bool countOk = n >= kMinDays && (n <= kMaxDays || (allowSeven && n == 7));
    if (longDay >= 7 || !((days >> longDay) & 1u) || !countOk) {
        return l;
    }
    l.ok = true;
    l.role[longDay] = Role::Long;

    // Rule 3: the best pair, when there are at least four run days.
    int bestA = -1, bestB = -1;
    if (n >= 4) {
        int bestGap = -1, bestFirst = 99;
        for (uint8_t x = 0; x < 7; ++x) {
            if (!((days >> x) & 1u) || !qualityAllowed(x, longDay, false)) {
                continue;
            }
            for (uint8_t y = 0; y < 7; ++y) {
                if (y == x || !((days >> y) & 1u) || !qualityAllowed(y, longDay, false)) {
                    continue;
                }
                const uint8_t ax = after(x, longDay), ay = after(y, longDay);
                if (ax >= ay || ay - ax == 1) {
                    continue;  // each pair once, earlier first; never next to each other
                }
                const int gap = smallestGap(ax, ay);
                if (gap > bestGap || (gap == bestGap && ax < bestFirst)) {
                    bestGap = gap;
                    bestFirst = ax;
                    bestA = x;
                    bestB = y;
                }
            }
        }
    }

    bool single = bestA < 0;
    if (!single) {
        l.role[bestA] = Role::Quality1;
        l.role[bestB] = Role::Quality2;
    } else {
        // Rules 3 and 4: one quality day, the allowed day furthest from L
        // (a tie: the sooner after L); the day before L only as a last resort.
        int best = -1, bestDist = -1, bestAfter = 99;
        for (int pass = 0; pass < 2 && best < 0; ++pass) {
            for (uint8_t x = 0; x < 7; ++x) {
                if (!((days >> x) & 1u) || !qualityAllowed(x, longDay, pass == 1)) {
                    continue;
                }
                const uint8_t a = after(x, longDay);
                const int dist = a < 7 - a ? a : 7 - a;
                if (dist > bestDist || (dist == bestDist && a < bestAfter)) {
                    best = x;
                    bestDist = dist;
                    bestAfter = a;
                }
            }
        }
        if (best < 0) {
            l = Layout{};
            return l;  // cannot happen with 3+ days, but never place nothing
        }
        l.role[best] = Role::KeyOnly;
    }

    // Rule 5: the rest, in order from the day after L: the easy slot first,
    // then (one quality day only, 4+ runs) the demoted second quality, then extras.
    bool easyGiven = false, demotedGiven = false;
    for (uint8_t k = 1; k < 7; ++k) {
        const uint8_t d = static_cast<uint8_t>((longDay + k) % 7u);
        if (!((days >> d) & 1u) || l.role[d] != Role::Rest) {
            continue;
        }
        if (!easyGiven) {
            l.role[d] = Role::Easy;
            easyGiven = true;
        } else if (single && !demotedGiven) {
            l.role[d] = Role::Demoted;
            demotedGiven = true;
        } else {
            l.role[d] = Role::Extra;
        }
    }
    return l;
}

void sessions(const Week& week, const Layout& layout, Session out[7])
{
    const bool threeDays = dayCount([&] {
        DayMask m = 0;
        for (uint8_t d = 0; d < 7; ++d) {
            if (layout.role[d] != Role::Rest) {
                m = static_cast<DayMask>(m | (1u << d));
            }
        }
        return m;
    }()) == 3;

    // With one quality day it holds the key session if the key is a quality
    // slot; otherwise (the key is the long slot) the first quality slot.
    const uint8_t keySlot = week.key == kSlotQ2 ? kSlotQ2 : kSlotQ1;
    const uint8_t otherSlot = keySlot == kSlotQ1 ? kSlotQ2 : kSlotQ1;
    const uint16_t easyMinutes = week.slot[kSlotEasy].a;

    for (uint8_t d = 0; d < 7; ++d) {
        Session s;
        switch (layout.role[d]) {
        case Role::Rest:
            break;
        case Role::Quality1:
            s = week.slot[kSlotQ1];
            break;
        case Role::Quality2:
            s = week.slot[kSlotQ2];
            break;
        case Role::KeyOnly:
            s = week.slot[keySlot];
            break;
        case Role::Easy:
            s = week.slot[kSlotEasy];
            if (threeDays) {
                s.kind = Kind::EasyStrides;  // rule 4: the other run has the strides
            }
            break;
        case Role::Demoted: {
            const Session& q = week.slot[otherSlot];
            if (isHard(q.kind)) {
                s.kind = Kind::EasyStrides;
                s.a = easyMinutes;
            } else {
                s = q;
            }
            break;
        }
        case Role::Extra:
            s.kind = Kind::Easy;
            s.a = kExtraEasyMinutes;
            break;
        case Role::Long:
            s = week.slot[kSlotLong];
            break;
        }
        out[d] = s;
    }
}

}  // namespace WeekLayout
}  // namespace XTraining
