/**
 ******************************************************************************
 * @file    ReviewInsight.cpp
 * @brief   What the reviews say together. See ReviewInsight.hpp.
 ******************************************************************************
 */

#include "ReviewInsight.hpp"

namespace XTraining {
namespace Review {

Insight analyse(const ReviewLog& log)
{
    Insight in;
    bool runOpen = true;
    bool first = true;
    uint32_t fastSum = 0, slowSum = 0;
    for (int i = static_cast<int>(log.count()) - 1; i >= 0; --i) {
        const ReviewEntry& e = log.at(static_cast<uint8_t>(i));
        if (!e.judged()) {
            continue;   // not judged: neither breaks nor extends a run
        }
        if (in.judged < 0xFFu) {
            ++in.judged;
        }
        if (first) {
            first = false;
            in.latestOn   = !e.offPace();
            in.latestFeel = e.feel;
        }
        if (!runOpen) {
            continue;
        }
        if (e.date <= log.handled || !e.offPace()) {
            runOpen = false;   // answered already, or on pace: the run ends here
            continue;
        }
        if (in.offRun == 0) {
            in.newest = e.date;
        }
        ++in.offRun;
        fastSum += e.fastPct;
        slowSum += e.slowPct();
        if (e.feel != 0) {
            ++in.asked;
            in.tired = static_cast<uint8_t>(in.tired + (isTired(e.feel) ? 1 : 0));
            in.fresh = static_cast<uint8_t>(in.fresh + (isFresh(e.feel) ? 1 : 0));
        }
    }
    if (in.offRun != 0) {
        in.dir = fastSum > slowSum ? Dir::Fast : Dir::Slow;   // a tie is slow: the safer thing to say
    }
    return in;
}

Line lineFor(const Insight& in)
{
    if (in.streak()) {
        if (in.dir == Dir::Fast) {
            return Line::Fast;
        }
        if (in.tired > in.fresh) {
            return Line::SlowTired;
        }
        if (in.fresh > in.tired) {
            return Line::SlowFresh;
        }
        return Line::SlowMixed;
    }
    if (in.judged != 0 && in.latestOn) {
        return isTired(in.latestFeel) ? Line::OnTired : Line::OnTrack;
    }
    return Line::None;   // one or two sessions off pace: nothing is said yet
}

}  // namespace Review
}  // namespace XTraining
