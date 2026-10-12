/**
 * make_log: write a sample weigh-in log, as the app writes it.
 *
 *   make_log <out.csv> [days=60] [seed=1]
 *
 * A slow loss of 0.35 kg a week from 84 kg, with the day-to-day swings a real
 * log shows (water, salt, a heavy meal), about one missed day in eight, and
 * one mistyped weigh-in corrected a minute later (the latest of a day counts).
 * Deterministic for a given seed. The log ends on 12 October 2026.
 *
 * For the W1 smoothing chart and, later, the simulator. Not real data.
 */

#include "Calendar.hpp"
#include "WeighLog.hpp"

#include <cstdio>
#include <cstdlib>

using namespace XWeighIn;

namespace
{
uint32_t gState = 1;

/// A uniform number in [0, 1), from a fixed LCG (Numerical Recipes).
double uniform()
{
    gState = gState * 1664525u + 1013904223u;
    return (gState >> 8) / 16777216.0;
}

/// Roughly normal, mean 0, standard deviation about 1 (sum of 4 uniforms).
double noise()
{
    return (uniform() + uniform() + uniform() + uniform() - 2.0) * 1.732;
}

bool writeLine(FILE* f, const Entry& e)
{
    char line[kMaxLineBytes];
    if (formatEntry(e, line, sizeof(line)) == 0) {
        return false;
    }
    return std::fprintf(f, "%s\n", line) > 0;
}
}  // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <out.csv> [days=60] [seed=1]\n", argv[0]);
        return 2;
    }
    const int days = argc > 2 ? std::atoi(argv[2]) : 60;
    gState = argc > 3 ? static_cast<uint32_t>(std::strtoul(argv[3], nullptr, 10)) : 1u;
    if (days < 1 || days > 3650) {
        std::fprintf(stderr, "days must be 1-3650\n");
        return 2;
    }

    FILE* f = std::fopen(argv[1], "w");
    if (!f) {
        std::perror(argv[1]);
        return 1;
    }
    std::fprintf(f, "%s\n", kLogHeader);

    const int32_t lastDay  = daysFromCivil(2026, 10, 12);
    const int32_t firstDay = lastDay - days + 1;
    const int     typoDay  = days / 2;
    int written = 0;
    for (int i = 0; i < days; ++i) {
        const double u = uniform();
        if (i != 0 && i != days - 1 && u < 0.125) {
            continue;   // a missed day
        }
        double kg = 84.0 - 0.05 * i + 0.35 * noise();
        if (uniform() < 0.08) {
            kg += 0.9;  // a salty meal or a late dinner the night before
        }
        Entry e;
        e.day        = firstDay + i;
        e.minute     = static_cast<uint16_t>(6 * 60 + 30 + static_cast<int>(uniform() * 60));
        e.hundredths = static_cast<int32_t>(kg * 10.0 + 0.5) * 10;   // the entry is in 0.1 kg steps

        if (i == typoDay) {
            // 10 kg out, then weighed again a minute later.
            Entry typo = e;
            typo.hundredths += 1000;
            writeLine(f, typo);
            ++written;
            e.minute = static_cast<uint16_t>(e.minute + 1);
        }
        if (!writeLine(f, e)) {
            std::fclose(f);
            return 1;
        }
        ++written;
    }
    std::fclose(f);
    std::fprintf(stderr, "%s: %d weigh-ins over %d days\n", argv[1], written, days);
    return 0;
}
