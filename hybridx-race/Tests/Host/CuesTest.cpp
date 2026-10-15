#include <gtest/gtest.h>

#include "Cues.hpp"

using namespace Race;
using namespace Race::Cues;

// Brief 8.3's shapes are unchanged by the pitch: a run is one beep, a station two, a Roxzone
// one short, the finish one long. They are what tells the cues apart on firmware without pitch.
TEST(Cues, BeepShapesAreBriefs)
{
    EXPECT_EQ(forSegment(SegmentType::Run).count, 1);
    EXPECT_EQ(forSegment(SegmentType::Station).count, 2);
    EXPECT_EQ(forSegment(SegmentType::RoxIn).count, 1);
    EXPECT_EQ(forSegment(SegmentType::RoxOut).count, 1);
    EXPECT_EQ(kFinish.count, 1);

    EXPECT_LT(kRoxzone.beepMs, kRun.beepMs);    // a Roxzone beep is the short one
    EXPECT_GT(kFinish.beepMs, kRun.beepMs);     // the finish is the long one
    EXPECT_GT(kFinish.beepMs, kStation.beepMs);
}

TEST(Cues, RoxzoneInAndOutSoundTheSame)
{
    const Cue in  = forSegment(SegmentType::RoxIn);
    const Cue out = forSegment(SegmentType::RoxOut);
    EXPECT_EQ(in.beepMs, out.beepMs);
    EXPECT_EQ(in.count, out.count);
    EXPECT_EQ(in.pitchHz, out.pitchHz);
}

// Pitch is what is new: every event has its own, inside the range the kernel honours (outside it
// the kernel plays its default, which would make two events sound alike).
TEST(Cues, EachEventHasItsOwnPitchInRange)
{
    const uint16_t pitches[] = {kRun.pitchHz, kStation.pitchHz, kRoxzone.pitchHz, kFinish.pitchHz};
    for (uint16_t p : pitches) {
        EXPECT_GE(p, kPitchMinHz);
        EXPECT_LE(p, kPitchMaxHz);
    }
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = i + 1; j < 4; ++j) {
            EXPECT_NE(pitches[i], pitches[j]) << i << " and " << j;
        }
    }
}

TEST(Cues, ACorruptSegmentTypeMakesNoSound)
{
    const Cue c = forSegment(static_cast<SegmentType>(200));
    EXPECT_EQ(c.count, 0);
    EXPECT_EQ(c.pitchHz, 0);
}
