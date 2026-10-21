// The save file: a round trip, a torn or nonsense file falls back to the
// backup, then a new game; values out of range or inconsistent are refused.

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "Run.hpp"
#include "SaveCodec.hpp"
#include "SaveStore.hpp"
#include "support/TreeFileSystem.hpp"

using namespace TrailQuest;

namespace
{

/// A real mid-game state: a new game played for a while, standing at a save point.
RunState played(uint32_t seed)
{
    TrailQuest::Run run;
    run.newGame(seed);
    RunState last = run.state();
    for (int step = 0; step < 120 && run.phase() != RunPhase::Finished; ++step) {
        switch (run.phase()) {
            case RunPhase::AtNode:
                if (run.wantsSave()) {
                    last = run.state();
                    run.saved();
                }
                run.travel(static_cast<uint8_t>(step % run.node().exitCount));
                break;
            case RunPhase::Obstacle:   run.attemptObstacle(BeatResult::Good); break;
            case RunPhase::InDuel: {
                PlayerTurn t;
                t.beat = (step % 3 == 0) ? BeatResult::Perfect : BeatResult::Good;
                run.duelTurn(t);
            } break;
            case RunPhase::JoinOffer:  run.acceptJoin(0); break;
            case RunPhase::StageClear: last = run.state(); run.nextStage(); break;
            default: break;
        }
    }
    return last;
}

std::string encoded(const RunState& s)
{
    char buf[SaveCodec::kMaxBytes];
    const size_t n = SaveCodec::encode(s, buf, sizeof(buf));
    return std::string(buf, n);
}

bool same(const RunState& a, const RunState& b)
{
    return encoded(a) == encoded(b);
}

/// Replace one line ("key=...") of an encoded save.
std::string withLine(std::string text, const std::string& key, const std::string& value)
{
    const size_t at = text.find(key + "=");
    if (at == std::string::npos) {
        return text;
    }
    const size_t end = text.find('\n', at);
    return text.replace(at, end - at, key + "=" + value);
}

} // namespace

TEST(Save, ANewGameIsAValidSave)
{
    TrailQuest::Run run;
    run.newGame(1);
    EXPECT_TRUE(SaveCodec::valid(run.state()));
}

TEST(Save, RealStatesRoundTripThroughTheText)
{
    for (uint32_t seed = 1; seed <= 30; ++seed) {
        const RunState s = played(seed);
        ASSERT_TRUE(SaveCodec::valid(s)) << "seed " << seed;
        const std::string text = encoded(s);
        ASSERT_FALSE(text.empty());
        ASSERT_LT(text.size(), SaveCodec::kMaxBytes);
        RunState back;
        ASSERT_TRUE(SaveCodec::decode(text.c_str(), text.size(), back)) << text;
        EXPECT_TRUE(same(s, back)) << "seed " << seed;
        EXPECT_EQ(back.rng, s.rng);
    }
}

TEST(Save, AResumedGameCarriesOnExactlyAsTheOriginal)
{
    // Save, load, then the same choices: the same game. The RNG travels too.
    const RunState s = played(17);
    const std::string text = encoded(s);
    RunState back;
    ASSERT_TRUE(SaveCodec::decode(text.c_str(), text.size(), back));
    TrailQuest::Run a, b;
    a.resume(s);
    b.resume(back);
    for (int i = 0; i < 30; ++i) {
        if (a.phase() == RunPhase::AtNode) {
            a.travel(0);
            b.travel(0);
        } else if (a.phase() == RunPhase::InDuel) {
            a.duelTurn(PlayerTurn{});
            b.duelTurn(PlayerTurn{});
        } else if (a.phase() == RunPhase::Obstacle) {
            a.attemptObstacle(BeatResult::Good);
            b.attemptObstacle(BeatResult::Good);
        } else {
            a.declineJoin();
            b.declineJoin();
            a.nextStage();
            b.nextStage();
        }
    }
    EXPECT_EQ(a.state().rng, b.state().rng);
    EXPECT_EQ(a.state().minutes, b.state().minutes);
    EXPECT_EQ(a.state().level, b.state().level);
}

TEST(Save, TheFileIsReadableText)
{
    TrailQuest::Run run;
    run.newGame(3);
    const std::string text = encoded(run.state());
    EXPECT_EQ(text.rfind("tq=1\n", 0), 0u);
    EXPECT_NE(text.find("\nbag=3,1,0,0,0\n"), std::string::npos);
    EXPECT_NE(text.find("\nstage=0\n"), std::string::npos);
}

TEST(Save, EncodingIntoTooSmallABufferFails)
{
    TrailQuest::Run run;
    run.newGame(3);
    char tiny[16];
    EXPECT_EQ(SaveCodec::encode(run.state(), tiny, sizeof(tiny)), 0u);
    EXPECT_EQ(tiny[0], '\0');
}

TEST(Save, MissingOrDoubledKeysAreRefused)
{
    TrailQuest::Run run;
    run.newGame(3);
    const std::string text = encoded(run.state());
    RunState out;
    std::string missing = text;
    missing.erase(missing.find("lap="), missing.find('\n', missing.find("lap=")) - missing.find("lap=") + 1);
    EXPECT_FALSE(SaveCodec::decode(missing.c_str(), missing.size(), out));
    const std::string doubled = text + "lv=2\n";
    EXPECT_FALSE(SaveCodec::decode(doubled.c_str(), doubled.size(), out));
    const std::string withUnknown = text + "future=7\n";
    EXPECT_TRUE(SaveCodec::decode(withUnknown.c_str(), withUnknown.size(), out));
    EXPECT_FALSE(SaveCodec::decode("", 0, out));
    EXPECT_FALSE(SaveCodec::decode(nullptr, 10, out));
}

TEST(Save, OutOfRangeAndInconsistentValuesAreRefused)
{
    TrailQuest::Run run;
    run.newGame(3);
    const std::string good = encoded(run.state());
    RunState out;
    ASSERT_TRUE(SaveCodec::decode(good.c_str(), good.size(), out));

    const struct { const char* key; const char* value; } bad[] = {
        { "tq", "2" },                // a future version
        { "rng", "0" },               // a stuck generator
        { "lv", "0" }, { "lv", "21" }, { "lv", "300" },
        { "xp", "999" },              // more than the level needs
        { "st", "0" }, { "st", "9999" },
        { "pc", "4" },
        { "pc", "1" },                // a partner count with no partner named (p0=255)
        { "bag", "7,0,0,0,0" },       // more than the bag holds
        { "bag", "1,1,1,1,2" },       // two headtorches
        { "bag", "1,1,1" },           // too few counts
        { "bag", "1,x,1,1,1" },
        { "stage", "5" },
        { "node", "8" },              // Tortoise pool, not yet beaten: that step ends in the duel
        { "offer", "0" },             // an offer at the trailhead, from a spirit never beaten
        { "offer", "4" },             // a boss never joins
        { "offer", "22" }, { "offer", "256" },
        { "node", "40" },
        { "hut", "1" },               // not a hut
        { "wx", "9" },
        { "lap", "10" },
        { "boss", "32" },
        { "met", "4194304" },         // a species that does not exist (bit 22)
        { "beat", "1" },              // beaten but never met
        { "min", "70000" },
        { "lv", "-1" }, { "lv", "1e3" }, { "lv", "" }, { "lv", "12345678901" },
    };
    for (const auto& b : bad) {
        const std::string text = withLine(good, b.key, b.value);
        EXPECT_FALSE(SaveCodec::decode(text.c_str(), text.size(), out)) << b.key << "=" << b.value;
    }
}

TEST(Save, AnOfferIsKeptOnlyWhereItMakesSense)
{
    TrailQuest::Run run;
    run.newGame(3);
    RunState s = run.state();
    s.node          = 1;   // the birch wood
    s.spiritsMet    = 1u << Spirit::RedFox;
    s.spiritsBeaten = 1u << Spirit::RedFox;
    s.offer         = Spirit::RedFox;
    EXPECT_TRUE(SaveCodec::valid(s));
    EXPECT_TRUE(SaveCodec::decode(encoded(s).c_str(), encoded(s).size(), s));

    RunState t = s;
    t.node = 3;   // the valley hut: no spirit was beaten there
    EXPECT_FALSE(SaveCodec::valid(t));
    t = s;
    t.spiritsBeaten = 0;   // never beaten
    t.spiritsMet    = 0;
    EXPECT_FALSE(SaveCodec::valid(t));
    t = s;
    t.partnerCount = 1;   // already running with you
    t.partners[0]  = Spirit::RedFox;
    EXPECT_FALSE(SaveCodec::valid(t));
}

TEST(Save, EveryStepThatAsksToSaveMakesAValidSave)
{
    // The service saves whenever the game asks. Each of those states, in many
    // games played many ways, must round-trip, or a save would be refused.
    int saves = 0;
    for (uint32_t seed = 1; seed <= 60; ++seed) {
        TrailQuest::Run run;
        run.newGame(seed);
        for (int step = 0; step < 400 && run.phase() != RunPhase::Finished; ++step) {
            if (run.wantsSave()) {
                const RunState s = run.state();
                const std::string text = encoded(s);
                RunState back;
                ASSERT_TRUE(SaveCodec::decode(text.c_str(), text.size(), back)) << "seed " << seed << ", step " << step
                                                                                 << "\n" << text;
                EXPECT_TRUE(same(s, back));
                run.saved();
                ++saves;
            }
            switch (run.phase()) {
                case RunPhase::AtNode:
                    run.travel(static_cast<uint8_t>((seed + static_cast<uint32_t>(step)) % run.node().exitCount));
                    break;
                case RunPhase::Obstacle:
                    run.attemptObstacle(step % 2 ? BeatResult::Miss : BeatResult::Good);
                    break;
                case RunPhase::InDuel: {
                    PlayerTurn t;
                    t.action = static_cast<Action>(static_cast<uint32_t>(step) % 3u);
                    t.beat   = step % 4 == 0 ? BeatResult::Miss : BeatResult::Good;
                    run.duelTurn(t);
                } break;
                case RunPhase::JoinOffer:
                    if (seed % 3 == 0) {
                        run.declineJoin();
                    } else {
                        run.acceptJoin(static_cast<uint8_t>(seed % kMaxPartners));
                    }
                    break;
                case RunPhase::StageClear: run.nextStage(); break;
                default: break;
            }
        }
    }
    EXPECT_GT(saves, 1000);
}

TEST(Save, APartnerMustBeARegularSpiritAndAppearOnce)
{
    TrailQuest::Run run;
    run.newGame(3);
    RunState s = run.state();
    s.partnerCount = 2;
    s.partners[0]  = Spirit::RedFox;
    s.partners[1]  = Spirit::RedFox;
    EXPECT_FALSE(SaveCodec::valid(s));
    s.partners[1] = Spirit::OldTortoise;
    EXPECT_FALSE(SaveCodec::valid(s));
    s.partners[1] = Spirit::BrownBear;
    EXPECT_TRUE(SaveCodec::valid(s));
}

TEST(Save, StoreSavesAndLoads)
{
    TreeFileSystem fs;
    EXPECT_EQ(SaveStore::load(fs).source, SaveStore::Source::None);
    const RunState s = played(5);
    ASSERT_TRUE(SaveStore::save(fs, s));
    const SaveStore::Loaded l = SaveStore::load(fs);
    EXPECT_EQ(l.source, SaveStore::Source::File);
    EXPECT_TRUE(same(l.state, s));
}

TEST(Save, ATornFileFallsBackToTheBackup)
{
    TreeFileSystem fs;
    const RunState first  = played(5);
    const RunState second = played(6);
    ASSERT_TRUE(SaveStore::save(fs, first));
    ASSERT_TRUE(SaveStore::save(fs, second));   // first is now the backup
    const std::string path = std::string("/Apps/TrailQuest/") + SaveStore::kFile;
    const std::string full = fs.content(path);
    fs.addFile(path, full.substr(0, full.size() / 2));   // torn mid-write
    const SaveStore::Loaded l = SaveStore::load(fs);
    EXPECT_EQ(l.source, SaveStore::Source::Backup);
    EXPECT_TRUE(same(l.state, first));
}

TEST(Save, NonsenseInBothGivesANewGame)
{
    TreeFileSystem fs;
    fs.addFile(std::string("/Apps/TrailQuest/") + SaveStore::kFile, "hello\n");
    fs.addFile(std::string("/Apps/TrailQuest/") + SaveStore::kFile + ".bak", std::string(400, 'x'));
    EXPECT_EQ(SaveStore::load(fs).source, SaveStore::Source::None);
}

TEST(Save, AnInvalidStateIsNeverWritten)
{
    TreeFileSystem fs;
    const RunState good = played(5);
    ASSERT_TRUE(SaveStore::save(fs, good));
    RunState bad = good;
    bad.level = 0;
    EXPECT_FALSE(SaveStore::save(fs, bad));
    EXPECT_TRUE(same(SaveStore::load(fs).state, good));
}

TEST(Save, SettingsDefaultToTheBeatAndRoundTrip)
{
    TreeFileSystem fs;
    EXPECT_TRUE(SaveStore::loadSettings(fs).beat);   // nothing saved: the beat is on
    SaveStore::Settings off;
    off.beat = false;
    ASSERT_TRUE(SaveStore::saveSettings(fs, off));
    EXPECT_FALSE(SaveStore::loadSettings(fs).beat);
    SaveStore::Settings on;
    ASSERT_TRUE(SaveStore::saveSettings(fs, on));
    EXPECT_TRUE(SaveStore::loadSettings(fs).beat);
    // A save game and the settings live in different files.
    EXPECT_EQ(SaveStore::load(fs).source, SaveStore::Source::None);
}
