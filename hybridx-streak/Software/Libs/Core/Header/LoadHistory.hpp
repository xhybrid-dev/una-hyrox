/**
 ******************************************************************************
 * @file    LoadHistory.hpp
 * @brief   Fitness, fatigue and form from the HybridX apps' training loads
 *          (hybridx-link TrainingLoad.hpp: Edwards' TRIMP per session). Pure
 *          C++, header-only, no heap, integer only.
 *
 * The model is Banister's, as TrainingPeaks' Performance Manager uses it (our
 * own names, not their trademarks):
 *   fitness(d) = fitness(d-1) + (load(d) - fitness(d-1)) / 42
 *   fatigue(d) = fatigue(d-1) + (load(d) - fatigue(d-1)) / 7
 *   form(d)    = fitness(d) - fatigue(d)
 * where load(d) is the sum of the day's session loads.
 *
 * Late sessions. Streak finds a session when it next opens, which can be days
 * after it happened. So the last kOpenDays days are kept as daily loads, still
 * open to a late session; the averages are kept settled up to the day before
 * the window, and each day that leaves the window is folded in. A session
 * older than the window (Streak not opened for over two weeks) is added to the
 * window's oldest day: close enough, and never lost.
 *
 * Fixed point: the averages are hundredths (x100), rounded at each step, so a
 * replay of the same loads gives the same numbers on the watch and in a test.
 *
 * The start. Both averages begin at 0, and fitness takes about six weeks to
 * build while fatigue takes one, so an athlete training steadily from day one
 * would read as overreaching for weeks. So when the first kWarmUpDays of
 * history are all in the window (openStart == firstDay), both averages are
 * seeded with those days' mean daily load: the best guess at the athlete's
 * usual level, from which form starts near zero and moves with the training.
 *
 * The words (provisional, for Jon to tune from real weeks; Streak NOTES
 * "Training load"): form as a share of fitness, above +10 % Fresh, -10 % to
 * +10 % Steady, -30 % to -10 % Building, below -30 % Overreaching. They show
 * only after kWarmUpDays of history; before that the word is Learning.
 * TrainingPeaks' fixed bands are for its own load scale and do not carry over.
 ******************************************************************************
 */

#ifndef STREAK_LOAD_HISTORY_HPP
#define STREAK_LOAD_HISTORY_HPP

#include <cstdint>

namespace Streak
{

constexpr uint8_t kOpenDays    = 14;   ///< days still open to a late session
constexpr int32_t kFitnessDays = 42;
constexpr int32_t kFatigueDays = 7;
constexpr int32_t kWarmUpDays  = 14;   ///< history before the form word means anything

/// What form says, in a word.
enum class FormWord : uint8_t { Learning = 0, NoLoad, Fresh, Steady, Building, Overreaching };

/// The load figures for a day, for the screens and the glance.
struct LoadFigures {
    uint32_t fitnessX100 = 0;
    uint32_t fatigueX100 = 0;
    int32_t  formX100    = 0;       ///< fitness - fatigue
    FormWord word        = FormWord::Learning;
    uint16_t historyDays = 0;       ///< days since the first day Streak kept loads, today included
};

/// The saved part: in State, and in the state file under "L" (StateCodec).
struct LoadState {
    bool     started     = false;
    int32_t  firstDay    = 0;       ///< the first day loads were kept (the warm-up counts from it)
    int32_t  openStart   = 0;       ///< the first open day; open days are [openStart, openStart + kOpenDays)
    uint32_t fitnessX100 = 0;       ///< settled: as of the end of day openStart - 1
    uint32_t fatigueX100 = 0;
    uint16_t daily[kOpenDays] = {}; ///< the open days' loads
    bool     seeded      = false;   ///< the averages have been seeded from the warm-up
};

static_assert(kOpenDays == kWarmUpDays, "the seed is the mean of the warm-up, which must all be in the window");

/// The days the graphs show (Streak NOTES "S8"): 12 weeks.
constexpr uint8_t kTrendDays = 84;

/// The settled days, a point a day, for the graphs (NOTES "S8"). Each day is recorded as it
/// leaves the open window, with the figures at the end of that day; the open days are replayed
/// when the graphs ask (LoadHistory::series). Saved in the state file under "H". Oldest first,
/// the days one after another, ending at lastDay.
struct LoadTrend {
    int32_t  lastDay = 0;                 ///< the newest point's day (when count > 0)
    uint8_t  count   = 0;
    uint16_t fitnessX10[kTrendDays] = {};
    uint16_t fatigueX10[kTrendDays] = {};
    uint16_t load[kTrendDays]       = {};

    void clear()
    {
        *this = LoadTrend();
    }

    /// Add @p day's figures. A day that does not follow the last one starts the history again
    /// (it cannot be filled in); one at or before it is ignored (it is there already).
    void record(int32_t day, uint32_t fitnessX100, uint32_t fatigueX100, uint16_t dayLoad)
    {
        if (count > 0 && day <= lastDay) {
            return;
        }
        if (count > 0 && day != lastDay + 1) {
            clear();
        }
        if (count == kTrendDays) {
            for (uint8_t i = 0; i + 1 < kTrendDays; ++i) {
                fitnessX10[i] = fitnessX10[i + 1];
                fatigueX10[i] = fatigueX10[i + 1];
                load[i]       = load[i + 1];
            }
            --count;
        }
        fitnessX10[count] = tenths(fitnessX100);
        fatigueX10[count] = tenths(fatigueX100);
        load[count]       = dayLoad;
        ++count;
        lastDay = day;
    }

    /// Hundredths to tenths, rounded, held to 16 bits.
    static constexpr uint16_t tenths(uint32_t x100)
    {
        const uint64_t t = (static_cast<uint64_t>(x100) + 5u) / 10u;   // no wrap at the top
        return static_cast<uint16_t>(t > 0xFFFFu ? 0xFFFFu : t);
    }
};

/// What the graphs draw: the days up to today, oldest first, ending at lastDay. A day before the
/// history began, or older than the history keeps, is simply not there (count is smaller).
struct LoadSeries {
    int32_t  lastDay     = 0;     ///< today
    uint8_t  count       = 0;     ///< days, one after another, ending at lastDay
    uint16_t historyDays = 0;     ///< as LoadFigures: under kWarmUpDays the lines are still learning
    uint16_t fitnessX10[kTrendDays] = {};
    uint16_t fatigueX10[kTrendDays] = {};
    uint16_t load[kTrendDays]       = {};
};

namespace LoadMath
{

/// One day's step of an average (hundredths), rounded half away from zero.
constexpr uint32_t step(uint32_t avgX100, uint32_t load, int32_t days)
{
    const int64_t target = static_cast<int64_t>(load) * 100;
    const int64_t diff   = target - static_cast<int64_t>(avgX100);
    const int64_t half   = days / 2;
    const int64_t delta  = diff >= 0 ? (diff + half) / days : -((-diff + half) / days);
    const int64_t next   = static_cast<int64_t>(avgX100) + delta;
    return next < 0 ? 0u : (next > INT32_MAX ? static_cast<uint32_t>(INT32_MAX) : static_cast<uint32_t>(next));
}

constexpr FormWord wordFor(uint32_t fitnessX100, uint32_t fatigueX100, uint16_t historyDays)
{
    if (historyDays < kWarmUpDays) {
        return FormWord::Learning;
    }
    if (fitnessX100 == 0 && fatigueX100 == 0) {
        return FormWord::NoLoad;
    }
    // form / fitness, in per cent, without dividing by zero (no fitness: any fatigue is a lot).
    const int64_t form = static_cast<int64_t>(fitnessX100) - static_cast<int64_t>(fatigueX100);
    const int64_t fit  = fitnessX100 == 0 ? 1 : static_cast<int64_t>(fitnessX100);
    if (form * 100 > 10 * fit) {
        return FormWord::Fresh;
    }
    if (form * 100 >= -10 * fit) {
        return FormWord::Steady;
    }
    if (form * 100 >= -30 * fit) {
        return FormWord::Building;
    }
    return FormWord::Overreaching;
}

} // namespace LoadMath

class LoadHistory
{
public:
    /// The longest gap folded day by day; beyond it both averages have long
    /// decayed to nothing (0.976^1000), so they are simply set to 0.
    static constexpr int32_t kMaxFold = 1000;

    /// @p trend, if given, records each day as it leaves the window (the model's own state does;
    /// a copy used only to read the figures for today does not).
    explicit LoadHistory(LoadState& s, LoadTrend* trend = nullptr) : mS(s), mTrend(trend) {}

    /// Move the window so @p today is its last day, folding the days that leave it.
    void advanceTo(int32_t today)
    {
        if (!mS.started) {
            mS = LoadState();
            mS.started   = true;
            mS.firstDay  = today;
            mS.openStart = today - (kOpenDays - 1);
            if (mTrend != nullptr) {
                mTrend->clear();
            }
            return;
        }
        const int32_t lastOpen = mS.openStart + (kOpenDays - 1);
        if (today <= lastOpen) {
            return;   // already there (or the clock went back: never move back)
        }
        int64_t shift = static_cast<int64_t>(today) - lastOpen;
        if (shift > kMaxFold) {
            mS.seeded      = true;   // nothing left to seed from
            mS.fitnessX100 = 0;
            mS.fatigueX100 = 0;
            for (uint8_t i = 0; i < kOpenDays; ++i) {
                mS.daily[i] = 0;
            }
            mS.openStart = today - (kOpenDays - 1);
            if (mTrend != nullptr) {
                mTrend->clear();   // years away: nothing to draw a line from
            }
            return;
        }
        for (; shift > 0; --shift) {
            maybeSeed();   // before the first warm-up day leaves the window
            const int32_t  day  = mS.openStart;
            const uint16_t load = mS.daily[0];
            fold(load);
            // The graphs' history: a settled day of the athlete's own (not a day before the
            // first, whose zeros only fill the window, nor one before the seed).
            if (mTrend != nullptr && mS.seeded && day >= mS.firstDay) {
                mTrend->record(day, mS.fitnessX100, mS.fatigueX100, load);
            }
            for (uint8_t i = 0; i + 1 < kOpenDays; ++i) {
                mS.daily[i] = mS.daily[i + 1];
            }
            mS.daily[kOpenDays - 1] = 0;
            ++mS.openStart;
        }
        maybeSeed();
    }

    /// A session's load on @p day. Call advanceTo(today) first. A day before the
    /// window goes on its oldest day; one after it (a clock problem) on its newest.
    void add(int32_t day, uint16_t load)
    {
        if (!mS.started || load == 0) {
            return;
        }
        int32_t i = day - mS.openStart;
        if (i < 0) {
            i = 0;
        } else if (i >= kOpenDays) {
            i = kOpenDays - 1;
        }
        const uint32_t sum = static_cast<uint32_t>(mS.daily[i]) + load;
        mS.daily[i] = static_cast<uint16_t>(sum > 0xFFFFu ? 0xFFFFu : sum);
    }

    /// The figures for the end of @p today (the window's days up to it applied).
    LoadFigures figures(int32_t today) const
    {
        LoadFigures f;
        if (!mS.started) {
            return f;
        }
        uint32_t fit = mS.fitnessX100;
        uint32_t fat = mS.fatigueX100;
        for (int32_t d = mS.openStart; d <= today && d < mS.openStart + kOpenDays; ++d) {
            const uint16_t load = mS.daily[d - mS.openStart];
            fit = LoadMath::step(fit, load, kFitnessDays);
            fat = LoadMath::step(fat, load, kFatigueDays);
        }
        f.fitnessX100 = fit;
        f.fatigueX100 = fat;
        f.formX100    = static_cast<int32_t>(fit) - static_cast<int32_t>(fat);
        const int32_t days = today - mS.firstDay + 1;
        f.historyDays = static_cast<uint16_t>(days < 0 ? 0 : (days > 0xFFFF ? 0xFFFF : days));
        f.word        = LoadMath::wordFor(fit, fat, f.historyDays);
        return f;
    }

    /// The load of @p day, if it is still in the window (else 0).
    uint16_t dayLoad(int32_t day) const
    {
        if (!mS.started || day < mS.openStart || day >= mS.openStart + kOpenDays) {
            return 0;
        }
        return mS.daily[day - mS.openStart];
    }

    /// The graphs' days up to @p today (call advanceTo(today) first): the settled days from
    /// @p trend, then the open days replayed. The settled part is used only when it runs up to the
    /// window (a history that ends elsewhere cannot be joined to it). Days before the first are
    /// left out.
    void series(const LoadTrend& trend, int32_t today, LoadSeries& out) const
    {
        out = LoadSeries();
        if (!mS.started || today < mS.openStart) {
            return;
        }
        const int32_t lastOpen = mS.openStart + (kOpenDays - 1);
        const int32_t end      = today < lastOpen ? today : lastOpen;
        const int32_t from     = maxOf(end - (kTrendDays - 1), mS.firstDay);   // the oldest day shown
        if (end < from) {
            return;
        }
        uint8_t n = 0;
        // The settled days, from the history, if it reaches the window.
        if (trend.count > 0 && trend.lastDay == mS.openStart - 1) {
            const int32_t first = trend.lastDay - (trend.count - 1);
            for (int32_t d = maxOf(first, from); d <= trend.lastDay && n < kTrendDays; ++d) {
                const uint8_t i  = static_cast<uint8_t>(d - first);
                out.fitnessX10[n] = trend.fitnessX10[i];
                out.fatigueX10[n] = trend.fatigueX10[i];
                out.load[n]       = trend.load[i];
                ++n;
            }
        } else {
            n = 0;   // no settled part: the line starts in the window
        }
        // The open days, replayed from the settled figures.
        uint32_t fit = mS.fitnessX100;
        uint32_t fat = mS.fatigueX100;
        for (int32_t d = mS.openStart; d <= end; ++d) {
            const uint16_t load = mS.daily[d - mS.openStart];
            fit = LoadMath::step(fit, load, kFitnessDays);
            fat = LoadMath::step(fat, load, kFatigueDays);
            if (d < from) {
                continue;
            }
            if (n == kTrendDays) {   // only when the settled part was a full 84 days: keep the newest
                for (uint8_t i = 0; i + 1 < kTrendDays; ++i) {
                    out.fitnessX10[i] = out.fitnessX10[i + 1];
                    out.fatigueX10[i] = out.fatigueX10[i + 1];
                    out.load[i]       = out.load[i + 1];
                }
                --n;
            }
            out.fitnessX10[n] = LoadTrend::tenths(fit);
            out.fatigueX10[n] = LoadTrend::tenths(fat);
            out.load[n]       = load;
            ++n;
        }
        out.count   = n;
        out.lastDay = end;
        const int32_t days = end - mS.firstDay + 1;
        out.historyDays = static_cast<uint16_t>(days < 0 ? 0 : (days > 0xFFFF ? 0xFFFF : days));
    }

    /// The loads of the days [from, to], summed (those still in the window).
    uint32_t sum(int32_t from, int32_t to) const
    {
        uint32_t s = 0;
        for (int32_t d = from; d <= to; ++d) {
            s += dayLoad(d);
        }
        return s;
    }

private:
    /// When the window holds exactly the warm-up, seed both averages with its mean.
    void maybeSeed()
    {
        if (mS.seeded || mS.openStart != mS.firstDay) {
            return;
        }
        uint32_t total = 0;
        for (uint8_t i = 0; i < kOpenDays; ++i) {
            total += mS.daily[i];
        }
        const uint32_t meanX100 = (total * 100u + kOpenDays / 2) / kOpenDays;
        mS.fitnessX100 = meanX100;
        mS.fatigueX100 = meanX100;
        mS.seeded      = true;
    }

    void fold(uint16_t load)
    {
        mS.fitnessX100 = LoadMath::step(mS.fitnessX100, load, kFitnessDays);
        mS.fatigueX100 = LoadMath::step(mS.fatigueX100, load, kFatigueDays);
    }

    static constexpr int32_t maxOf(int32_t a, int32_t b) { return a > b ? a : b; }

    LoadState& mS;
    LoadTrend* mTrend = nullptr;
};

} // namespace Streak

#endif // STREAK_LOAD_HISTORY_HPP
