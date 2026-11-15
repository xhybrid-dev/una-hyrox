/**
 * @file    plan_print.cpp
 * @brief   Print a whole X Training plan for one runner, as Markdown.
 *
 *     plan_print --plan 10k|half|vo2max --5k 22:00 [--half 1:45:00]
 *                [--level 1|2|3] [--days tue,wed,fri,sun] [--long sun]
 *                [--start 2027-01-04] [--race 2027-03-14]
 *                [--not-recent] [--hr] [--distance]
 *
 * Defaults: level 2, Tuesday, Wednesday, Friday and Sunday, long run on
 * Sunday, set up on Monday 4 January 2027, no race date.
 */

#include "PlanPrint.hpp"
#include "TimeText.hpp"

#include <cstdio>
#include <cstring>
#include <string>

using namespace XTraining;

namespace {

int dayIndex(const std::string& s)
{
    static const char* const kNames[7] = {"mon", "tue", "wed", "thu", "fri", "sat", "sun"};
    for (int i = 0; i < 7; ++i) {
        if (s == kNames[i]) {
            return i;
        }
    }
    return -1;
}

int usage()
{
    std::fprintf(stderr,
                 "usage: plan_print --plan 10k|half|vo2max --5k mm:ss [--half h:mm:ss] [--level 1-3]\n"
                 "                  [--days tue,wed,fri,sun] [--long sun] [--start YYYY-MM-DD]\n"
                 "                  [--race YYYY-MM-DD] [--not-recent] [--hr] [--distance]\n");
    return 2;
}

}  // namespace

int main(int argc, char** argv)
{
    PrintInput in;
    bool havePlan = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const char* v = i + 1 < argc ? argv[i + 1] : nullptr;
        if (a == "--not-recent") {
            in.fiveKRecent = false;
        } else if (a == "--hr") {
            in.easyByHr = true;
        } else if (a == "--distance") {
            in.repsByDistance = true;
        } else if (v == nullptr) {
            return usage();
        } else if (a == "--plan") {
            const std::string p = v;
            in.plan = p == "10k" ? PlanId::TenK : p == "half" ? PlanId::Half : p == "vo2max" ? PlanId::Vo2max : PlanId::None;
            havePlan = in.plan != PlanId::None;
            ++i;
        } else if (a == "--5k") {
            in.fiveKSec = TimeText::parseMinSec(v);
            ++i;
        } else if (a == "--half") {
            in.halfSec = TimeText::parseHms(v);
            ++i;
        } else if (a == "--level") {
            in.level = static_cast<uint8_t>(std::atoi(v));
            ++i;
        } else if (a == "--days") {
            in.days = 0;
            std::string s = v;
            size_t start = 0;
            while (start <= s.size()) {
                const size_t comma = s.find(',', start);
                const std::string d = s.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
                const int idx = dayIndex(d);
                if (idx < 0) {
                    return usage();
                }
                in.days = static_cast<DayMask>(in.days | (1u << idx));
                if (comma == std::string::npos) {
                    break;
                }
                start = comma + 1;
            }
            ++i;
        } else if (a == "--long") {
            const int idx = dayIndex(v);
            if (idx < 0) {
                return usage();
            }
            in.longDay = static_cast<uint8_t>(idx);
            ++i;
        } else if (a == "--start") {
            in.setupDay = TimeText::parseDate(v);
            ++i;
        } else if (a == "--race") {
            in.raceDate = TimeText::parseDate(v);
            ++i;
        } else {
            return usage();
        }
    }
    if (!havePlan || in.fiveKSec == 0 || in.level < 1 || in.level > 3 || in.setupDay == 0) {
        return usage();
    }
    const std::string out = printPlan(in);
    std::fwrite(out.data(), 1, out.size(), stdout);
    return 0;
}
