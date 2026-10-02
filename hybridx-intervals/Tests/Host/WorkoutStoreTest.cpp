#include <gtest/gtest.h>

#include <string>

#include "WorkoutStore.hpp"
#include "support/FlatFileSystem.hpp"

using namespace Intervals;

namespace
{
std::string workout(const std::string& name, const std::string& sport = "run",
                    const std::string& steps = "{\"type\":\"time\",\"value\":60}")
{
    return "{\"v\":1,\"name\":\"" + name + "\",\"sport\":\"" + sport + "\",\"steps\":[" + steps + "]}";
}

const WorkoutInfo* find(const WorkoutStore& store, const char* file)
{
    for (uint8_t i = 0; i < store.count(); ++i) {
        if (std::string(store.infos()[i].file) == file) {
            return &store.infos()[i];
        }
    }
    return nullptr;
}

int indexOf(const WorkoutStore& store, const char* file)
{
    for (uint8_t i = 0; i < store.count(); ++i) {
        if (std::string(store.infos()[i].file) == file) {
            return i;
        }
    }
    return -1;
}
} // namespace

TEST(WorkoutStore, AnEmptyWatchGetsAWorkoutsFolder)
{
    FlatFileSystem fs;
    WorkoutStore   store(fs);
    EXPECT_EQ(store.scan(), 0u);
    EXPECT_TRUE(fs.exist("Workouts"));
    EXPECT_FALSE(store.loaded());
    EXPECT_FALSE(store.restoreSelection());
}

TEST(WorkoutStore, ListsJsonFilesSortedByTheNameInside)
{
    FlatFileSystem fs;
    fs.addDir("Workouts");
    fs.addFile("Workouts/b.json", workout("tempo 5k"));
    fs.addFile("Workouts/a.json", workout("Zone 2"));
    fs.addFile("Workouts/c.JSON", workout("Hills"));
    fs.addFile("Workouts/notes.txt", "not a workout");
    fs.addFile("Workouts/._a.json", "macOS junk");
    fs.addDir("Workouts/sub.json");
    WorkoutStore store(fs);
    ASSERT_EQ(store.scan(), 3u);
    EXPECT_STREQ(store.infos()[0].name, "Hills");
    EXPECT_STREQ(store.infos()[1].name, "tempo 5k");
    EXPECT_STREQ(store.infos()[2].name, "Zone 2");
    for (uint8_t i = 0; i < 3; ++i) {
        EXPECT_TRUE(store.infos()[i].runnable);
        EXPECT_EQ(store.infos()[i].summary.timeS, 60u);
    }
}

TEST(WorkoutStore, ABrokenFileIsListedByFileNameWithItsReason)
{
    FlatFileSystem fs;
    fs.addFile("Workouts/Broken one.json", "{\"v\":1,\"name\":");
    fs.addFile("Workouts/future.json", "{\"v\":2}");
    fs.addFile("Workouts/bad repeat.json", workout("x", "run", "{\"type\":\"repeat\",\"from\":0,\"count\":2}"));
    WorkoutStore store(fs);
    ASSERT_EQ(store.scan(), 3u);

    const WorkoutInfo* broken = find(store, "Broken one.json");
    ASSERT_NE(broken, nullptr);
    EXPECT_STREQ(broken->name, "Broken one");
    EXPECT_EQ(broken->error, ParseError::Syntax);
    EXPECT_FALSE(broken->runnable);
    EXPECT_GT(broken->errorAt, 0u);

    EXPECT_EQ(find(store, "future.json")->error, ParseError::BadVersion);
    const WorkoutInfo* bad = find(store, "bad repeat.json");
    EXPECT_EQ(bad->error, ParseError::Invalid);
    EXPECT_EQ(bad->validation, ValidationError::RepeatIndexNotBefore);

    EXPECT_FALSE(store.load(static_cast<uint8_t>(indexOf(store, "future.json"))));
    EXPECT_FALSE(store.loaded());
}

TEST(WorkoutStore, ATooLargeFileIsRefusedWithoutReadingIt)
{
    FlatFileSystem fs;
    std::string    big = workout("big");
    big += std::string(kMaxWorkoutFileBytes, ' ');
    fs.addFile("Workouts/big.json", big);
    WorkoutStore store(fs);
    ASSERT_EQ(store.scan(), 1u);
    EXPECT_EQ(store.infos()[0].error, ParseError::TooLarge);
}

TEST(WorkoutStore, BikeWorkoutsAreListedButNotRunnable)
{
    FlatFileSystem fs;
    fs.addFile("Workouts/spin.json", workout("Spin", "bike"));
    WorkoutStore store(fs);
    ASSERT_EQ(store.scan(), 1u);
    EXPECT_EQ(store.infos()[0].error, ParseError::Ok);
    EXPECT_EQ(store.infos()[0].sport, Sport::Cycling);
    EXPECT_FALSE(store.infos()[0].runnable);
    EXPECT_FALSE(store.load(0));
}

TEST(WorkoutStore, LoadRemembersTheChoiceForTheNextStart)
{
    FlatFileSystem fs;
    fs.addFile("Workouts/a.json", workout("Alpha"));
    fs.addFile("Workouts/b.json", workout("Bravo", "run", "{\"type\":\"dist\",\"value\":400}"));
    {
        WorkoutStore store(fs);
        store.scan();
        ASSERT_TRUE(store.load(1));
        EXPECT_TRUE(store.loaded());
        EXPECT_STREQ(store.current().name, "Bravo");
        EXPECT_EQ(store.current().steps[0].durationValue, 40000u);
        EXPECT_STREQ(store.currentInfo().file, "b.json");
        EXPECT_EQ(fs.content("workout.sel"), "b.json");
    }
    WorkoutStore again(fs);
    again.scan();
    ASSERT_TRUE(again.restoreSelection());
    EXPECT_EQ(again.selected(), 1);
    EXPECT_STREQ(again.current().name, "Bravo");
}

TEST(WorkoutStore, ForgetClearsTheChoiceOnFile)
{
    FlatFileSystem fs;
    fs.addFile("Workouts/a.json", workout("Alpha"));
    WorkoutStore store(fs);
    store.scan();
    ASSERT_TRUE(store.load(0));
    store.forget();
    EXPECT_FALSE(store.loaded());
    EXPECT_FALSE(fs.hasFile("workout.sel"));
    EXPECT_FALSE(store.restoreSelection());
}

TEST(WorkoutStore, ARescanFollowsTheLoadedWorkoutAndPicksUpANewVersion)
{
    FlatFileSystem fs;
    fs.addFile("Workouts/m.json", workout("Mike"));
    WorkoutStore store(fs);
    store.scan();
    ASSERT_TRUE(store.load(0));
    // A new file sorts before it, and the loaded file is replaced by a new version.
    fs.addFile("Workouts/a.json", workout("Alpha"));
    fs.addFile("Workouts/m.json", workout("Mike", "run", "{\"type\":\"time\",\"value\":120}"));
    store.scan();
    ASSERT_TRUE(store.loaded());
    EXPECT_EQ(store.selected(), 1);
    EXPECT_EQ(store.current().steps[0].durationValue, 120000u);
}

TEST(WorkoutStore, ARescanDropsALoadedWorkoutThatWasDeletedOrBroken)
{
    FlatFileSystem fs;
    fs.addFile("Workouts/m.json", workout("Mike"));
    WorkoutStore store(fs);
    store.scan();
    ASSERT_TRUE(store.load(0));
    fs.addFile("Workouts/m.json", "{broken");
    store.scan();
    EXPECT_FALSE(store.loaded());

    fs.addFile("Workouts/m.json", workout("Mike"));
    store.scan();
    ASSERT_TRUE(store.load(0));
    fs.remove("Workouts/m.json");
    store.scan();
    EXPECT_FALSE(store.loaded());
    EXPECT_EQ(store.count(), 0u);
}

TEST(WorkoutStore, AtMostSixteenAreListedAndTheRestFlagged)
{
    FlatFileSystem fs;
    for (int i = 0; i < 20; ++i) {
        char file[32];
        std::snprintf(file, sizeof(file), "Workouts/w%02d.json", i);
        fs.addFile(file, workout("W" + std::to_string(i)));
    }
    WorkoutStore store(fs);
    EXPECT_EQ(store.scan(), WorkoutStore::kMaxWorkouts);
    EXPECT_TRUE(store.truncated());
}

TEST(WorkoutStore, ALongFileNameIsSkippedNotTruncated)
{
    FlatFileSystem fs;
    fs.addFile("Workouts/" + std::string(60, 'a') + ".json", workout("long"));
    fs.addFile("Workouts/ok.json", workout("ok"));
    WorkoutStore store(fs);
    EXPECT_EQ(store.scan(), 1u);
    EXPECT_STREQ(store.infos()[0].file, "ok.json");
}
