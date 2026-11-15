/**
 ******************************************************************************
 * @file    TrackSummaryScreen.cpp
 * @brief   Activity summary (see TrackSummaryScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackSummaryScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Format.hpp"
#include "gui/Strings.hpp"

#include "FormText.hpp"   // HybridX Run
#include "RunStatsText.hpp"   // HybridX Run
#include "Vo2Text.hpp"   // HybridX Run

using namespace SDK::GUI;
using F = Theme::Font;

TrackSummaryScreen::TrackSummaryScreen(Model& model)
    : Screen(model)
{
}

void TrackSummaryScreen::build()
{
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::NONE, Widgets::Buttons::NONE);
    mIndicator = std::make_unique<Widgets::ScrollIndicator>(mRoot, Widgets::ScrollIndicator::kSmall);
    mIndicator->setCount(FACE_LAPS);
}

// -- One face at a time (HybridX Run) ------------------------------------------

void TrackSummaryScreen::destroyFace()
{
    if (!mFace) {
        return;
    }
    mTitle.reset();
    mMap.reset();
    lv_obj_delete(mFace);
    mFace = nullptr;

    mMapDistance = mMapUnits = mOverviewDistance = mOverviewUnits = mAvgPace = mTimer = nullptr;
    mMaxHr = mAvgHr = nullptr;
    mVo2Run = mVo2RunLabel = mVo2RecentLabel = mVo2Confidence = mVo2MaxHr = nullptr;
    mFormCadence = mFormStep = nullptr;
    for (auto& o : mZoneBar) {
        o = nullptr;
    }
    for (auto& o : mZoneTime) {
        o = nullptr;
    }
    mZoneBelow = nullptr;
    mBestTime[0] = mBestTime[1] = mBestTime[2] = nullptr;
    mHalfPace[0] = mHalfPace[1] = nullptr;
    mGapPace = mDrift = mDriftNote = nullptr;
    mLapsCount = mLapsTotal = nullptr;
    for (auto& row : mLapRows) {
        for (auto& cell : row) {
            cell = nullptr;
        }
    }
}

void TrackSummaryScreen::buildFace(uint8_t face)
{
    destroyFace();
    switch (face) {
        case FACE_MAP:       buildFaceMap(); break;
        case FACE_OVERVIEW:  buildFaceOverview(); break;
        case FACE_HEARTRATE: buildFaceHeartRate(); break;
        case FACE_VO2:       buildFaceVo2(); break;
        case FACE_FORM:      buildFaceForm(); break;
        case FACE_ZONES:     buildFaceZones(); break;
        case FACE_SPLITS:    buildFaceSplits(); break;
        case FACE_PACING:    buildFacePacing(); break;
        case FACE_LAPS:      buildFaceLaps(); break;
        default:             return;
    }
    // Behind the scroll indicator and the button hints.
    lv_obj_move_background(mFace);
    fillFace();
    ScreenManager::logPool("summary face");
}

void TrackSummaryScreen::buildFaceMap()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    mMapUnits    = Theme::label(f, F::Medium18, Strings::kKm, 178, 73, 45, LV_TEXT_ALIGN_LEFT);
    mMapDistance = Theme::label(f, F::SemiBold40, Strings::kNoValue, 60, 51, 120);
    Theme::image(f, &img_runningman_30x30, 33, 62);
    if (mModel.getTrackData().treadmill) {
        // HybridX Run: a treadmill run has no route to draw.
        Theme::label(f, F::Medium18, "No map on a\ntreadmill", 45, 120, 150, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        mTitle = std::make_unique<Widgets::Title>(f, "TREADMILL");
        return;
    }
    mMap = std::make_unique<Widgets::Map>(f, 45, 90);
    mTitle = std::make_unique<Widgets::Title>(f, "MAP");
}

void TrackSummaryScreen::buildFaceOverview()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    mTimer = Theme::label(f, F::SemiBold25, "0:00:00", 95, 173, 117);
    Theme::label(f, F::Medium18, "TIMER", 43, 179, 51);
    Theme::hline(f, 25, 166, 190);
    // Wider than the design's 78 px box (same centre) so "22:00" fits.
    mAvgPace = Theme::label(f, F::SemiBold25, Strings::kNoValue, 121, 127, 100);
    Theme::label(f, F::Medium18, "AVG PACE", 39, 133, 100);
    Theme::hline(f, 25, 120, 190);
    mOverviewUnits    = Theme::label(f, F::Medium18, Strings::kKm, 178, 73, 45, LV_TEXT_ALIGN_LEFT);
    mOverviewDistance = Theme::label(f, F::SemiBold40, Strings::kNoValue, 60, 51, 120);
    Theme::image(f, &img_runningman_30x30, 33, 62);
    mTitle = std::make_unique<Widgets::Title>(f, "SUMMARY");
}

void TrackSummaryScreen::buildFaceHeartRate()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    Theme::label(f, F::SemiBold20, "MAX HR", 81, 118, 78);
    mMaxHr = Theme::label(f, F::SemiBold40, Strings::kNoValue, 70, 77, 100);
    Theme::hline(f, 25, 147, 190);
    Theme::label(f, F::SemiBold20, "AVG HR", 81, 191, 75);
    mAvgHr = Theme::label(f, F::SemiBold40, Strings::kNoValue, 70, 150, 100);
    Theme::image(f, &img_heart_30x30, 105, 48);
    mTitle = std::make_unique<Widgets::Title>(f, "HEART RATE");
}

// HybridX Run: this run's VO2max (ml/kg/min) and the recent figure. Without
// an estimate, the reason sits where "THIS RUN" would.
void TrackSummaryScreen::buildFaceVo2()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    Theme::label(f, F::Regular14, "ml/kg/min", 70, 52, 100, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mVo2Run      = Theme::label(f, F::SemiBold40, Strings::kNoValue, 70, 77, 100);
    mVo2RunLabel = Theme::label(f, F::SemiBold20, "THIS RUN", 20, 118, 200);
    Theme::hline(f, 25, 147, 190);
    mVo2RecentLabel = Theme::label(f, F::Medium18, "", 20, 160, 200);
    // P3: how much to trust this run's figure. Narrower lower down: the round face's chord is
    // about 150 px at the second line's foot.
    mVo2Confidence = Theme::label(f, F::Regular14, "", 30, 184, 180, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mVo2MaxHr      = Theme::label(f, F::Regular14, "", 45, 200, 150, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mTitle = std::make_unique<Widgets::Title>(f, "VO2MAX");
}

// HybridX Run: the run's average cadence and step length, laid out as the heart rate face.
void TrackSummaryScreen::buildFaceForm()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    Theme::label(f, F::SemiBold20, "CADENCE", 60, 118, 120);   // wider than "MAX HR"'s box: "CADENCE" is clipped by 78 px
    mFormCadence = Theme::label(f, F::SemiBold40, Strings::kNoValue, 70, 77, 100);
    Theme::label(f, F::Regular18, RunVo2::FormText::kCadenceUnit, 172, 99, 45, LV_TEXT_ALIGN_LEFT);
    Theme::hline(f, 25, 147, 190);
    Theme::label(f, F::SemiBold20, "STEP LENGTH", 50, 191, 140);
    mFormStep = Theme::label(f, F::SemiBold40, Strings::kNoValue, 70, 150, 100);
    Theme::label(f, F::Regular18, RunVo2::FormText::kStepUnit, 172, 172, 45, LV_TEXT_ALIGN_LEFT);
    mTitle = std::make_unique<Widgets::Title>(f, "FORM");
}

namespace
{
// The zone colours of the heart-rate arc on the lap face (Widgets.cpp), zone 1 to 5.
constexpr uint32_t kZoneColour[5] = {
    SDK::GUI::Color::GRAY, SDK::GUI::Color::CHARTREUSE, SDK::GUI::Color::YELLOW,
    SDK::GUI::Color::YELLOW_DARK, SDK::GUI::Color::RED,
};
constexpr int32_t kZoneBarX   = 66;
constexpr int32_t kZoneBarMax = 86;   // the widest bar, in pixels
}  // namespace

// HybridX Run: time in the watch's heart-rate zones, a bar and a time each. Inside the round
// face: rows from y 44 to 188, x 30 to 210 (the chord is 188 px wide at the bottom row).
void TrackSummaryScreen::buildFaceZones()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    for (uint8_t i = 0; i < kZoneRows; ++i) {
        const int32_t y = 44 + i * 30;
        char name[4];
        snprintf(name, sizeof(name), "Z%u", static_cast<unsigned>(i + 1));
        Theme::label(f, F::Medium18, name, 28, y, 34, LV_TEXT_ALIGN_LEFT, kZoneColour[i]);
        mZoneBar[i]  = Theme::box(f, kZoneBarX, y + 4, 4, 16, kZoneColour[i], 3);
        mZoneTime[i] = Theme::label(f, F::Regular16, "", 144, y + 2, 58, LV_TEXT_ALIGN_RIGHT);   // ends short of the amber arc
    }
    mZoneBelow = Theme::label(f, F::Regular14, "", 20, 192, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mTitle = std::make_unique<Widgets::Title>(f, "ZONES");
}

// HybridX Run: the fastest 1 km, 1 mile and 5 km of the run, and the pace of each half.
void TrackSummaryScreen::buildFaceSplits()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    static const char* const kLabel[3] = {"1 km", "1 mi", "5 km"};
    for (uint8_t i = 0; i < 3; ++i) {
        const int32_t y = 46 + i * 34;
        Theme::label(f, F::Medium18, kLabel[i], 34, y + 5, 46, LV_TEXT_ALIGN_LEFT);
        // HybridX Run: "PB" beside the distance when this run set the personal best.
        mBestTag[i]  = Theme::label(f, F::Medium18, "", 88, y + 5, 30, LV_TEXT_ALIGN_LEFT, Color::YELLOW_DARK);
        mBestTime[i] = Theme::label(f, F::SemiBold25, Strings::kNoValue, 106, y, 90, LV_TEXT_ALIGN_RIGHT);
    }
    Theme::hline(f, 35, 150, 170);
    Theme::label(f, F::Regular14, "HALF PACE", 20, 156, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mHalfPace[0] = Theme::label(f, F::SemiBold20, Strings::kNoValue, 28, 176, 88);
    mHalfPace[1] = Theme::label(f, F::SemiBold20, Strings::kNoValue, 124, 176, 88);
    Theme::vline(f, 119, 172, 28);
    mTitle = std::make_unique<Widgets::Title>(f, "BEST SPLITS");
}

// HybridX Run: the run's grade-adjusted pace, and the drift of heart rate against pace.
void TrackSummaryScreen::buildFacePacing()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    Theme::label(f, F::Regular14, "ADJUSTED PACE", 20, 46, 190, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mGapPace = Theme::label(f, F::SemiBold35, Strings::kNoValue, 40, 62, 160);
    Theme::hline(f, 35, 108, 170);
    Theme::label(f, F::Regular14, "DRIFT", 20, 116, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mDrift     = Theme::label(f, F::SemiBold35, Strings::kNoValue, 40, 132, 160);
    mDriftNote = Theme::label(f, F::Regular16, "", 20, 178, 200);
    mTitle = std::make_unique<Widgets::Title>(f, "PACING");
}

void TrackSummaryScreen::buildFaceLaps()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    mLapsCount = Theme::label(f, F::SemiBold30, "0 LAPS", 20, 40, 200);
    Theme::image(f, &img_clock_16x19, 54, 75);
    mLapsTotal = Theme::label(f, F::Regular18, "Total: 0:00:00", 73, 75, 118);
    Theme::hline(f, 25, 100, 190);
    Theme::label(f, F::Regular16, "Distance", 33, 104, 80, LV_TEXT_ALIGN_LEFT);
    Theme::label(f, F::Regular16, "Time", 119, 104, 45, LV_TEXT_ALIGN_LEFT, Color::CYAN_PURE);
    Theme::label(f, F::Regular16, "Pace", 169, 104, 44, LV_TEXT_ALIGN_LEFT);

    // Five visible rows, refilled on each page, like the TouchGFX ScrollList's
    // recycled drawables. Columns: index, distance, units, time, pace.
    lv_obj_t* list = Theme::container(f, 25, 126, 190, 90);
    for (uint8_t r = 0; r < kLapVisible; ++r) {
        const int32_t y = r * 18;
        mLapRows[r][0] = Theme::label(list, F::Regular14, "", 0,   y, 21, LV_TEXT_ALIGN_RIGHT);
        mLapRows[r][1] = Theme::label(list, F::Regular14, "", 21,  y, 40, LV_TEXT_ALIGN_RIGHT);
        mLapRows[r][2] = Theme::label(list, F::Regular14, "", 64,  y, 28, LV_TEXT_ALIGN_LEFT);
        mLapRows[r][3] = Theme::label(list, F::Regular14, "", 98,  y, 45, LV_TEXT_ALIGN_LEFT, Color::CYAN_PURE);
        mLapRows[r][4] = Theme::label(list, F::Regular14, "", 145, y, 45, LV_TEXT_ALIGN_LEFT);
    }
    mTitle = std::make_unique<Widgets::Title>(f, "LAPS");
}

void TrackSummaryScreen::onShow()
{
    mModel.resetIdleTimer();
    mIsImperial = mModel.isUnitsImperial();
    mPaused     = mModel.isTrackPaused();
    mSummary    = mModel.isTrackSummaryAvailable() ? &mModel.getTrackSummary() : nullptr;
    if (mSummary) {
        setSummary(*mSummary);
    }
    // While paused the way back is R2 (to the action menu); after a save R1 leaves.
    if (mPaused) {
        mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                      Widgets::Buttons::NONE, Widgets::Buttons::AMBER);
    } else {
        mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                      Widgets::Buttons::AMBER, Widgets::Buttons::NONE);
    }
    showFace(FACE_MAP);
}

void TrackSummaryScreen::onActivitySummary(const ActivitySummary& summary)
{
    mSummary = &summary;
    setSummary(summary);   // back to the first lap page...
    fillFace();
    updateIndicator();     // ...and the indicator with it
}

void TrackSummaryScreen::onRunStats(const RunVo2::RunStatsResult& /*stats*/)
{
    if (mFaceId == FACE_ZONES || mFaceId == FACE_SPLITS || mFaceId == FACE_PACING) {
        fillFace();
    }
}

void TrackSummaryScreen::onBests()
{
    if (mFaceId == FACE_SPLITS) {
        fillFace();
    }
}

void TrackSummaryScreen::onVo2Info(const CustomMessage::Vo2Info& /*info*/)
{
    if (mFaceId == FACE_VO2) {
        fillFace();
    }
}

void TrackSummaryScreen::setSummary(const ActivitySummary& s)
{
    // What every face shares: the lap pages behind the indicator.
    mLaps     = &s.laps;
    mLapPage  = 0;
    mLapPages = s.laps.empty() ? 0 : static_cast<uint8_t>((s.laps.size() + kLapPageSize - 1) / kLapPageSize);
    mIndicator->setCount(static_cast<uint16_t>(FACE_LAPS + mLapPages));
}

void TrackSummaryScreen::fillFace()
{
    if (!mFace) {
        return;
    }
    char buf[24];

    if (mFaceId == FACE_VO2) {   // HybridX Run
        const CustomMessage::Vo2Info& info = mModel.getVo2Info();
        char v[8];
        if (mModel.getTrackData().treadmill) {
            // HybridX Run: no VO2max from a treadmill run (NOTES, "Treadmill"); the recent
            // figure below is still the outdoor runs'.
            lv_label_set_text(mVo2Run, Strings::kNoValue);
            lv_label_set_text(mVo2RunLabel, "Not on treadmill");
            lv_obj_set_style_text_font(mVo2RunLabel, Theme::font(F::Regular16), 0);
        } else if (!info.hasRun) {
            // Paused: the estimate is made when the run is saved.
            lv_label_set_text(mVo2Run, Strings::kNoValue);
            lv_label_set_text(mVo2RunLabel, "After saving");
        } else {
            RunVo2::RunResult r;
            r.status        = static_cast<RunVo2::RunStatus>(info.runStatus);
            r.profileStatus = static_cast<RunVo2::ProfileStatus>(info.profileStatus);
            lv_label_set_text(mVo2Run, RunVo2::Text::formatX10(info.runX10, v, sizeof(v)));
            const char* why = RunVo2::Text::reason(r);
            lv_label_set_text(mVo2RunLabel, why[0] ? why : "THIS RUN");
            lv_obj_set_style_text_font(mVo2RunLabel, Theme::font(why[0] ? F::Regular16 : F::SemiBold20), 0);
        }
        // P3: the steady minutes behind this run's figure and how far they spread, and the max HR.
        if (!info.hasRun || info.runX10 == 0) {
            lv_label_set_text(mVo2Confidence, "");
        } else {
            char sp[8];
            // Short: the worst case, "240 min, spread 99.9", is about 145 px (checked in the simulator).
            lv_label_set_text_fmt(mVo2Confidence, "%u min, spread %s", static_cast<unsigned>(info.windowsUsed),
                                  RunVo2::Text::formatX10(info.spreadX10, sp, sizeof(sp)));
        }
        if (!info.hasRun || info.maxHr == 0) {
            lv_label_set_text(mVo2MaxHr, "");
        } else {
            const char* src = RunVo2::Text::maxSourceLabel(static_cast<RunVo2::MaxHrSource>(info.maxSource));
            if (src[0]) {
                lv_label_set_text_fmt(mVo2MaxHr, "Max HR %u (%s)", static_cast<unsigned>(info.maxHr), src);
            } else {
                lv_label_set_text_fmt(mVo2MaxHr, "Max HR %u", static_cast<unsigned>(info.maxHr));
            }
        }
        if (info.rollingX10 == 0) {
            lv_label_set_text(mVo2RecentLabel, "");
        } else {
            const unsigned runs = info.runs < RunVo2::Config::kRollingRuns ? info.runs : RunVo2::Config::kRollingRuns;
            lv_label_set_text_fmt(mVo2RecentLabel, "Recent %s (%u %s)",
                                  RunVo2::Text::formatX10(info.rollingX10, v, sizeof(v)), runs,
                                  runs == 1 ? "run" : "runs");
        }
        return;
    }

    if (mFaceId == FACE_ZONES || mFaceId == FACE_SPLITS || mFaceId == FACE_PACING) {   // HybridX Run
        fillDetail();
        return;
    }

    if (!mSummary) {
        return;
    }
    const ActivitySummary& s = *mSummary;
    switch (mFaceId) {
        case FACE_MAP:
        case FACE_OVERVIEW: {
            const float dist = Fmt::distUnits(s.distance, mIsImperial);
            Fmt::distanceTotal(buf, sizeof(buf), dist);
            if (mFaceId == FACE_MAP) {
                lv_label_set_text(mMapDistance, buf);
                lv_label_set_text(mMapUnits, Fmt::units(mIsImperial));
                if (mMap) {
                    mMap->setMap(s.map);
                }
            } else {
                lv_label_set_text(mOverviewDistance, buf);
                lv_label_set_text(mOverviewUnits, Fmt::units(mIsImperial));
                Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(s.paceAvg, mIsImperial));
                lv_label_set_text(mAvgPace, buf);
                Fmt::hms(buf, sizeof(buf), s.time);
                lv_label_set_text(mTimer, buf);
            }
        } break;

        case FACE_HEARTRATE:
            Fmt::heartRate(buf, sizeof(buf), s.hrMax);
            lv_label_set_text(mMaxHr, buf);
            Fmt::heartRate(buf, sizeof(buf), s.hrAvg);
            lv_label_set_text(mAvgHr, buf);
            break;

        case FACE_FORM:
            RunVo2::FormText::cadence(buf, sizeof(buf), static_cast<uint16_t>(s.cadenceAvgSpm * 10u));
            lv_label_set_text(mFormCadence, buf);
            RunVo2::FormText::stepLength(buf, sizeof(buf), s.stepLengthAvgMm);
            lv_label_set_text(mFormStep, buf);
            break;

        case FACE_LAPS: {
            lv_label_set_text_fmt(mLapsCount, "%lu LAPS", static_cast<unsigned long>(s.laps.size()));
            std::time_t total = 0;
            for (const auto& lap : s.laps) {
                total += lap.duration;
            }
            char t[16];
            Fmt::hms(t, sizeof(t), total);
            snprintf(buf, sizeof(buf), "Total: %s", t);
            lv_label_set_text(mLapsTotal, buf);
            fillLapRows();
        } break;

        default:
            break;
    }
}

// HybridX Run: the run detail faces, from the service's RunStatsUpd (sent at pause and at save).
void TrackSummaryScreen::fillDetail()
{
    const RunVo2::RunStatsResult& r = mModel.getRunStats();
    char buf[24];

    if (mFaceId == FACE_ZONES) {
        uint32_t most = 0;
        for (uint8_t i = 1; i <= kZoneRows; ++i) {
            most = r.zoneSec[i] > most ? r.zoneSec[i] : most;
        }
        for (uint8_t i = 0; i < kZoneRows; ++i) {
            const uint32_t sec = r.zoneSec[i + 1];
            // A bar is at least a dot wide if the zone was used at all; the longest fills kZoneBarMax.
            const int32_t w = sec == 0 || most == 0 ? 0 : 4 + static_cast<int32_t>((kZoneBarMax - 4) * static_cast<uint64_t>(sec) / most);
            lv_obj_set_width(mZoneBar[i], w > 0 ? w : 1);
            if (w == 0) {
                lv_obj_add_flag(mZoneBar[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_remove_flag(mZoneBar[i], LV_OBJ_FLAG_HIDDEN);
            }
            if (r.zoneCount == 0) {
                lv_label_set_text(mZoneTime[i], Strings::kNoValue);
            } else {
                Fmt::shortTime(buf, sizeof(buf), static_cast<std::time_t>(sec));
                lv_label_set_text(mZoneTime[i], buf);
            }
        }
        if (r.zoneCount == 0 || r.zoneSec[0] == 0) {
            lv_label_set_text(mZoneBelow, "");
        } else {
            char t[12];   // "h:mm" or "m:ss" (shortTime); "Below Z1  " + 11 fits the 24-byte buf
            Fmt::shortTime(t, sizeof(t), static_cast<std::time_t>(r.zoneSec[0]));
            snprintf(buf, sizeof(buf), "Below Z1  %s", t);
            lv_label_set_text(mZoneBelow, buf);
        }
        return;
    }

    if (mFaceId == FACE_SPLITS) {
        const uint8_t newMask = mModel.getBests().newMask;
        for (uint8_t i = 0; i < 3; ++i) {
            lv_label_set_text(mBestTag[i], (newMask & (1u << i)) ? "PB" : "");
            if (r.bestMs[i] == 0) {
                lv_label_set_text(mBestTime[i], Strings::kNoValue);
            } else {
                Fmt::shortTime(buf, sizeof(buf), static_cast<std::time_t>((r.bestMs[i] + 500u) / 1000u));
                lv_label_set_text(mBestTime[i], buf);
            }
        }
        for (uint8_t h = 0; h < 2; ++h) {
            if (r.halfPace[h] <= 0.0f) {
                lv_label_set_text(mHalfPace[h], Strings::kNoValue);
            } else {
                Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(r.halfPace[h], mIsImperial));
                lv_label_set_text(mHalfPace[h], buf);
            }
        }
        return;
    }

    // FACE_PACING. HybridX Run: a treadmill run's speed has no grade to adjust for.
    if (r.gapSpeedMmps == 0 || mModel.getTrackData().treadmill) {
        lv_label_set_text(mGapPace, Strings::kNoValue);
    } else {
        Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(1000.0f / static_cast<float>(r.gapSpeedMmps), mIsImperial));
        lv_label_set_text(mGapPace, buf);
    }
    RunVo2::StatsText::drift(buf, sizeof(buf), r);
    lv_label_set_text(mDrift, buf);
    const char* why = RunVo2::StatsText::driftReason(r.driftStatus);
    lv_label_set_text(mDriftNote, why[0] ? why : "after the warm-up");
}

void TrackSummaryScreen::fillLapRows()
{
    if (mFaceId != FACE_LAPS || !mFace) {
        return;
    }
    const size_t first = static_cast<size_t>(mLapPage) * kLapPageSize;
    for (uint8_t r = 0; r < kLapVisible; ++r) {
        const size_t idx = first + r;
        if (!mLaps || idx >= mLaps->size()) {
            for (uint8_t c = 0; c < kLapColumns; ++c) {
                lv_label_set_text(mLapRows[r][c], "");
            }
            continue;
        }
        const LapSummary& lap = (*mLaps)[idx];
        char buf[16];
        snprintf(buf, sizeof(buf), "%lu.", static_cast<unsigned long>(idx + 1));
        lv_label_set_text(mLapRows[r][0], buf);
        const float dist = Fmt::distUnits(lap.distance, mIsImperial);
        Fmt::fixed(buf, sizeof(buf), dist, dist < 100.0f ? 2 : 1);
        lv_label_set_text(mLapRows[r][1], buf);
        lv_label_set_text(mLapRows[r][2], Fmt::units(mIsImperial));
        Fmt::shortTime(buf, sizeof(buf), lap.duration);
        lv_label_set_text(mLapRows[r][3], buf);
        Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(lap.paceAvg, mIsImperial));
        lv_label_set_text(mLapRows[r][4], buf);
    }
}

void TrackSummaryScreen::showFace(uint8_t face)
{
    mFaceId = face;
    buildFace(face);
    updateIndicator();
}

void TrackSummaryScreen::updateIndicator()
{
    const uint8_t id = mFaceId < FACE_LAPS ? mFaceId : static_cast<uint8_t>(FACE_LAPS + mLapPage);
    mIndicator->setActive(id);
}

void TrackSummaryScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L2:
            if (mFaceId == FACE_LAPS) {
                if (mLapPage + 1 < mLapPages) {
                    ++mLapPage;
                    fillLapRows();
                    updateIndicator();
                }
            } else if (mFaceId < FACE_PACING || mLapPages > 0) {
                showFace(mFaceId + 1);
            }
            break;

        case Btn::L1:
            if (mFaceId == FACE_LAPS) {
                if (mLapPage > 0) {
                    --mLapPage;
                    fillLapRows();
                    updateIndicator();
                } else {
                    showFace(mFaceId - 1);
                }
            } else if (mFaceId > FACE_MAP) {
                showFace(mFaceId - 1);
            }
            break;

        case Btn::R1:
            if (!mPaused) {
                backToTrack();
            }
            break;

        case Btn::R2:
            if (mPaused) {
                backToTrack();
            }
            break;

        default:
            break;
    }
}

void TrackSummaryScreen::backToTrack()
{
    if (mModel.isTrackPaused()) {
        ScreenManager::instance().goTo(ScreenId::TrackAction);
    } else {
        mModel.exitApp();
    }
}
