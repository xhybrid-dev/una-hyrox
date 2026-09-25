/**
 * make_state: write a HybridX Streak state.json with a history, so the
 * simulator can open onto moments that take weeks to reach for real: a missed
 * week with shields to spend, or a summit one session away.
 *
 *   make_state <out.json> <weeks achieved> <weeks missed after them>
 *
 * The weeks run up to the one before this one (weeks start on Monday); each
 * achieved week has three sessions from an app folder "Earlier". The model
 * does the rest when the simulator opens: missed weeks become a shield offer.
 */

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

#include "StateCodec.hpp"
#include "StreakModel.hpp"
#include "WeekMath.hpp"

int main(int argc, char** argv)
{
    if (argc < 4) {
        std::fprintf(stderr, "usage: make_state <out.json> <weeks achieved> <weeks missed>\n");
        return 2;
    }
    const int achieved = std::atoi(argv[2]);
    const int missed   = std::atoi(argv[3]);

    const std::time_t utc = std::time(nullptr);
    std::tm           local {};
    localtime_r(&utc, &local);
    const int32_t today  = Streak::WeekMath::daysFromCivil(local.tm_year + 1900, static_cast<uint32_t>(local.tm_mon + 1),
                                                           static_cast<uint32_t>(local.tm_mday));
    const int32_t monday = Streak::WeekMath::periodStartDay(Streak::WeekMath::periodOf(today, 1), 1);
    const int32_t first  = monday - 7 * (achieved + missed);

    Streak::StreakModel m;
    m.reset(Streak::Goal {});
    Streak::Events ev;
    m.update(first, nullptr, 0, ev);
    for (int w = 0; w < achieved; ++w) {
        const int32_t      start = first + 7 * w;
        std::vector<Streak::Found> f(3);
        for (int i = 0; i < 3; ++i) {
            f[i].appKey     = Streak::ActivityScanner::appKey("Earlier");
            f[i].localDay   = start + 2 * i;
            f[i].localStart = static_cast<uint32_t>(start + 2 * i) * 86400u + 7u * 3600u;
            f[i].kind       = i == 1 ? Streak::Kind::Strength : Streak::Kind::Run;
            f[i].minutes    = 40;
            snprintf(f[i].app, sizeof(f[i].app), "Earlier");
        }
        Streak::Events e;
        m.update(start + 6, f.data(), f.size(), e);
    }

    static char buf[Streak::StateCodec::kMaxBytes];
    const size_t len = Streak::StateCodec::encode(m.state(), buf, sizeof(buf));
    FILE*        out = len ? std::fopen(argv[1], "wb") : nullptr;
    if (!out || std::fwrite(buf, 1, len, out) != len) {
        std::fprintf(stderr, "failed to write %s\n", argv[1]);
        return 1;
    }
    std::fclose(out);
    std::printf("%d weeks achieved, streak %u, %u shields, %d missed to come\n", achieved,
                static_cast<unsigned>(m.state().streak + (m.weekMet() ? 1 : 0)), static_cast<unsigned>(m.state().shields), missed);
    return 0;
}
