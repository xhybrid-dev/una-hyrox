// The pace model held to Daniels' own tables: every row of Tables 5.1, 5.2 and 5.3 of
// Daniels' Running Formula, 4th edition (2022), read from the book by hand into
// fixtures/daniels_vdot.csv. Each row's 5K time goes in; the training paces and predicted
// race times that come out are compared with the book's.

#include "PaceModel.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace XTraining;

namespace {

// "m:ss", "h:mm:ss" or "m:ss.s" in seconds; -1 for "-".
double seconds(const std::string& t)
{
    if (t.empty() || t == "-") {
        return -1.0;
    }
    double part[3] = {0, 0, 0};
    int n = 0;
    std::stringstream ss(t);
    std::string x;
    while (std::getline(ss, x, ':') && n < 3) {
        part[n++] = std::atof(x.c_str());
    }
    return n == 2 ? part[0] * 60 + part[1] : part[0] * 3600 + part[1] * 60 + part[2];
}

struct Row {
    int vdot;
    double fiveK, tenK, half, marathon, eFast, eSlow, mKm, tKm, iKm, i400, r400, r200;
};

std::vector<Row> rows()
{
    std::vector<Row> out;
    std::ifstream f(std::string(XTRAINING_FIXTURE_DIR) + "/daniels_vdot.csv");
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == 'v') {
            continue;
        }
        std::vector<std::string> c;
        std::stringstream ss(line);
        std::string x;
        while (std::getline(ss, x, ',')) {
            c.push_back(x);
        }
        if (c.size() != 13) {
            ADD_FAILURE() << "bad row: " << line;
            continue;
        }
        out.push_back({std::atoi(c[0].c_str()), seconds(c[1]), seconds(c[2]), seconds(c[3]), seconds(c[4]),
                       seconds(c[5]), seconds(c[6]), seconds(c[7]), seconds(c[8]), seconds(c[9]),
                       seconds(c[10]), seconds(c[11]), seconds(c[12])});
    }
    return out;
}

}  // namespace

TEST(Daniels, TheFixtureHasEveryRow)
{
    const std::vector<Row> r = rows();
    ASSERT_EQ(r.size(), 66u);   // VDOT 20 to 85
    EXPECT_EQ(r.front().vdot, 20);
    EXPECT_EQ(r.back().vdot, 85);
}

TEST(Daniels, TrainingPacesMatchTables52And53)
{
    for (const Row& b : rows()) {
        const PaceResult r = PaceModel::compute({static_cast<uint32_t>(b.fiveK + 0.5), 0});
        ASSERT_TRUE(r.ok) << b.vdot;
        const Paces& p = r.paces;
        // Threshold: the book to the second, all the way down.
        EXPECT_NEAR(p.threshold, b.tKm, 2) << "T at VDOT " << b.vdot;
        // Easy (Table 5.2 only; Table 5.3 gives none).
        if (b.eFast > 0) {
            EXPECT_NEAR(p.easyFast, b.eFast, 3) << "E fast at VDOT " << b.vdot;
            EXPECT_NEAR(p.easySlow, b.eSlow, 3) << "E slow at VDOT " << b.vdot;
        }
        // Interval: per km where the book gives it, else from its 400 m time.
        const double iKm = b.iKm > 0 ? b.iKm : b.i400 * 2.5;
        EXPECT_NEAR(p.interval, iKm, 3) << "I at VDOT " << b.vdot;
        // Repetition: from 400 m, or 200 m below VDOT 34 (whole seconds per 200 m: +/-2.5 s/km).
        const double rKm = b.r400 > 0 ? b.r400 * 2.5 : b.r200 * 5.0;
        EXPECT_NEAR(p.repetition, rKm, 5) << "R at VDOT " << b.vdot;
    }
}

TEST(Daniels, PredictedRacesMatchTable51)
{
    // Table 5.1 is the Daniels-Gilbert equations' own output: within a quarter of a per cent.
    for (const Row& b : rows()) {
        if (b.tenK < 0) {
            continue;   // Table 5.3's rows give only the mile and 5K
        }
        const PaceResult r = PaceModel::compute({static_cast<uint32_t>(b.fiveK + 0.5), 0});
        EXPECT_NEAR(r.predicted10kSec, b.tenK, b.tenK * 0.0025) << "10K at VDOT " << b.vdot;
        EXPECT_NEAR(r.predictedHalfSec, b.half, b.half * 0.0025) << "half at VDOT " << b.vdot;
        EXPECT_NEAR(r.predictedMarathonSec, b.marathon, b.marathon * 0.0025) << "marathon at VDOT " << b.vdot;
    }
}

TEST(Daniels, MarathonTrainingPaceIsTheBooksForGoodRunners)
{
    // Our "marathon" pace is the predicted race's (shown, not used by the plans). From VDOT 45
    // it is the book's M training pace too; below, Daniels' M is slower than his own predicted
    // marathon (by up to 18 s/km at VDOT 20), which the plans never need.
    for (const Row& b : rows()) {
        if (b.vdot < 45) {
            continue;
        }
        const PaceResult r = PaceModel::compute({static_cast<uint32_t>(b.fiveK + 0.5), 0});
        EXPECT_NEAR(r.paces.marathon, b.mKm, 2) << "M at VDOT " << b.vdot;
    }
}
