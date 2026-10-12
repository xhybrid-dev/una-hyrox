/**
 * trend_csv: read a weigh-in log and print it day by day with the trend.
 *
 *   trend_csv <weighins.csv>
 *
 * stdout, a CSV: date,weight,trend,mean7 for each day the trend covers (the
 * last 90 days at most). weight is that day's counted weigh-in (the latest),
 * empty on a missed day. mean7 is a plain mean of the weigh-ins in the 7 days
 * up to that day, for comparison only: the app does not use it.
 *
 * stderr: the summary the app would show.
 *
 * Reads with the app's own parser and trend (Core), so what it prints is what
 * the watch would compute from the same file.
 */

#include "Calendar.hpp"
#include "Trend.hpp"
#include "WeighLog.hpp"
#include "WeighText.hpp"

#include <cstdio>
#include <fstream>
#include <map>
#include <string>

using namespace XWeighIn;

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <weighins.csv>\n", argv[0]);
        return 2;
    }
    std::ifstream in(argv[1]);
    if (!in) {
        std::perror(argv[1]);
        return 1;
    }

    Trend                      trend;
    std::map<int32_t, int32_t> latestOfDay;   // for mean7
    uint32_t                   rejected = 0;
    std::string                line;
    while (std::getline(in, line)) {
        Entry e;
        if (parseLine(line.data(), line.size(), e)) {
            trend.onEntry(e);
            latestOfDay[e.day] = e.hundredths;
        } else if (!line.empty() && line != "\r" && line.rfind(kLogHeader, 0) != 0) {
            ++rejected;
        }
    }

    const Summary s = trend.summary();
    if (s.days == 0) {
        std::fprintf(stderr, "no weigh-ins (%u lines rejected)\n", rejected);
        return 1;
    }

    std::printf("date,weight,trend,mean7\n");
    char kg[16];
    for (int32_t day = s.lastDay - (kHistoryDays - 1); day <= s.lastDay; ++day) {
        DayPoint p;
        if (!trend.at(day, p)) {
            continue;
        }
        const Date d = civilFromDays(day);
        std::printf("%04d-%02u-%02u,", static_cast<int>(d.year),
                    static_cast<unsigned>(d.month), static_cast<unsigned>(d.day));
        if (p.hundredths > 0) {
            formatKg(p.hundredths, 2, kg, sizeof(kg));
            std::printf("%s", kg);
        }
        formatKg(p.trend, 2, kg, sizeof(kg));
        std::printf(",%s,", kg);

        long sum = 0;
        int  n   = 0;
        for (auto it = latestOfDay.lower_bound(day - 6); it != latestOfDay.end() && it->first <= day; ++it) {
            sum += it->second;
            ++n;
        }
        if (n > 0) {
            formatKg(static_cast<int32_t>((sum + n / 2) / n), 2, kg, sizeof(kg));
            std::printf("%s", kg);
        }
        std::printf("\n");
    }

    char a[16], b[16];
    formatKg(s.latest, 2, a, sizeof(a));
    formatKg(s.trend, 2, b, sizeof(b));
    std::fprintf(stderr, "%u days weighed, latest %s kg, trend %s kg", s.days, a, b);
    if (s.hasWeekChange) {
        formatChange(s.weekChange, 2, a, sizeof(a));
        std::fprintf(stderr, ", %s kg this week", a);
    }
    std::fprintf(stderr, "; %u skipped, %u lines rejected\n", s.skipped, rejected);
    return 0;
}
