// review_print: a reviews.json read with the core's own reader, what ReviewInsight says of it (TP8),
// and a maker of seeded ones for the simulator.
//
//   review_print <reviews.json> [--expect-count N] [--expect-newest judged|unjudged]
//                [--expect-dir fast|slow|none] [--expect-feel Name|none] [--expect-offer yes|no]
//                [--expect-handled yyyymmdd] [--expect-off-run N]
//   review_print --make <out.json> [--handled yyyymmdd] <date:in:fast:feel> ...
//
// The expectations are about the newest entry (its direction is where its misses lean) and what the
// insight makes of the whole log. Exit 0 if the file reads and every expectation holds, 1 if not.
// --make writes a log of sessions: in and fast are the shares in band and too fast (the rest is slow),
// feel is 0 to 5 (not asked, Fresh, Good, Steady, Tired, Spent); a share of 0:0 is not judged.

#include "PlanText.hpp"
#include "ReviewInsight.hpp"
#include "ReviewLog.hpp"
#include "ReviewText.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace XTraining;

namespace {

const char* dirName(Review::Dir d)
{
    return d == Review::Dir::Fast ? "fast" : d == Review::Dir::Slow ? "slow" : "none";
}

bool readFile(const char* path, std::string& out)
{
    FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        return false;
    }
    char buf[512];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) {
        out.append(buf, n);
    }
    std::fclose(f);
    return true;
}

int make(int argc, char** argv)
{
    // review_print --make out.json [--handled d] entry...
    const char* out = argv[2];
    ReviewLog log;
    for (int i = 3; i < argc; ++i) {
        if (std::strcmp(argv[i], "--handled") == 0 && i + 1 < argc) {
            log.handled = static_cast<Ymd>(std::strtoul(argv[++i], nullptr, 10));
            continue;
        }
        unsigned date = 0, in = 0, fast = 0, feel = 0;
        if (std::sscanf(argv[i], "%u:%u:%u:%u", &date, &in, &fast, &feel) != 4 || !Date::valid(date) || in + fast > 100
            || feel >= Review::kFeelCount) {
            std::fprintf(stderr, "bad entry: %s (date:in:fast:feel)\n", argv[i]);
            return 1;
        }
        ReviewEntry e;
        e.date      = date;
        e.kind      = Kind::Intervals;
        e.judgedSec = (in == 0 && fast == 0) ? 0 : 900;
        e.inPct     = static_cast<uint8_t>(in);
        e.fastPct   = static_cast<uint8_t>(fast);
        e.gapSecKm  = e.judged() ? (fast * 2 > (100 - in) ? -8 : 8) : 0;
        e.feel      = static_cast<uint8_t>(feel);
        log.record(e);
    }
    char buf[ReviewLog::kMaxJsonBytes];
    const size_t n = log.toJson(buf, sizeof(buf));
    if (n == 0) {
        std::fprintf(stderr, "does not fit\n");
        return 1;
    }
    FILE* f = std::fopen(out, "wb");
    if (f == nullptr || std::fwrite(buf, 1, n, f) != n) {
        std::fprintf(stderr, "cannot write %s\n", out);
        return 1;
    }
    std::fclose(f);
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc >= 3 && std::strcmp(argv[1], "--make") == 0) {
        return make(argc, argv);
    }
    if (argc < 2) {
        std::fprintf(stderr, "usage: review_print <reviews.json> [--expect-...]; review_print --make <out> <date:in:fast:feel>...\n");
        return 2;
    }
    std::string text;
    if (!readFile(argv[1], text)) {
        std::printf("FAIL reviews.json: cannot read %s\n", argv[1]);
        return 1;
    }
    ReviewLog log;
    if (!log.fromJson(text.c_str(), text.size())) {
        std::printf("FAIL reviews.json: not a valid file\n");
        return 1;
    }
    std::printf("reviews.json: %u sessions, suggestion answered on %lu\n", static_cast<unsigned>(log.count()),
                static_cast<unsigned long>(log.handled));
    for (uint8_t i = 0; i < log.count(); ++i) {
        const ReviewEntry& e = log.at(i);
        if (e.judged()) {
            std::printf("  %lu %-14s %3u%% in band, %u%% fast, %u%% slow, gap %d s/km, fade %d, %u s judged, felt %s\n",
                        static_cast<unsigned long>(e.date), PlanText::name(e.kind), static_cast<unsigned>(e.inPct),
                        static_cast<unsigned>(e.fastPct), static_cast<unsigned>(e.slowPct()), static_cast<int>(e.gapSecKm),
                        static_cast<int>(e.fadeSecKm), static_cast<unsigned>(e.judgedSec),
                        e.feel != 0 ? ReviewText::feelName(e.feel) : "(not asked)");
        } else {
            std::printf("  %lu %-14s not judged, felt %s\n", static_cast<unsigned long>(e.date), PlanText::name(e.kind),
                        e.feel != 0 ? ReviewText::feelName(e.feel) : "(not asked)");
        }
    }
    const Review::Insight in = Review::analyse(log);
    std::printf("insight: %u judged, %u in a row off pace (%s), suggestion %s, line %u\n", static_cast<unsigned>(in.judged),
                static_cast<unsigned>(in.offRun), dirName(in.dir), in.offerEase() ? "hold the plan" : (in.streak() ? "say so" : "none"),
                static_cast<unsigned>(Review::lineFor(in)));

    bool ok = true;
    auto check = [&](const char* name, bool pass, const std::string& detail) {
        std::printf("%s %s: %s\n", pass ? "PASS" : "FAIL", name, detail.c_str());
        ok = ok && pass;
    };
    const ReviewEntry newest = log.count() > 0 ? log.at(static_cast<uint8_t>(log.count() - 1)) : ReviewEntry{};
    for (int i = 2; i + 1 < argc; i += 2) {
        const std::string k = argv[i];
        const std::string v = argv[i + 1];
        if (k == "--expect-count") {
            check("count", log.count() == std::strtoul(v.c_str(), nullptr, 10), std::to_string(log.count()) + " vs " + v);
        } else if (k == "--expect-newest") {
            const bool want = v == "judged";
            check("newest", log.count() > 0 && newest.judged() == want, newest.judged() ? "judged" : "not judged");
        } else if (k == "--expect-dir") {
            check("direction", dirName(newest.dir()) == v, std::string(dirName(newest.dir())) + " vs " + v);
        } else if (k == "--expect-feel") {
            std::string got = newest.feel != 0 ? ReviewText::feelName(newest.feel) : "none";
            check("feel", got == v, got + " vs " + v);
        } else if (k == "--expect-offer") {
            const bool want = v == "yes";
            check("offer", in.offerEase() == want, in.offerEase() ? "hold the plan" : "none");
        } else if (k == "--expect-handled") {
            check("answered", log.handled == std::strtoul(v.c_str(), nullptr, 10), std::to_string(log.handled) + " vs " + v);
        } else if (k == "--expect-off-run") {
            check("off run", in.offRun == std::strtoul(v.c_str(), nullptr, 10), std::to_string(in.offRun) + " vs " + v);
        } else {
            std::printf("FAIL unknown option %s\n", k.c_str());
            ok = false;
        }
    }
    return ok ? 0 : 1;
}
