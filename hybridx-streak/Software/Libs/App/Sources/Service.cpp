/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   HybridX Streak's service process (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstring>
#include <ctime>
#include <new>

#include "SDK/Messages/MessageGuard.hpp"

#include "ActivityScanner.hpp"
#include "AppConfigFields.hpp"
#include "StateCodec.hpp"
#include "StreakModel.hpp"
#include "WeekMath.hpp"

#define LOG_MODULE_PRX   "Service"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

using Effect = SDK::Message::RequestVibroPlay::Effect;

namespace
{
constexpr const char* kStateFile  = "state.json";
constexpr const char* kSharedDir  = "../SharedData/HybridX";
constexpr const char* kPublicFile = "../SharedData/HybridX/streak.json";
constexpr size_t      kMaxFound   = Streak::ActivityScanner::kMaxNew;

// Placed at start-up (see the header): far larger than the service stack.
alignas(Streak::StreakModel) uint8_t     sModelStorage[sizeof(Streak::StreakModel)];
alignas(Streak::ActivityScanner) uint8_t sScannerStorage[sizeof(Streak::ActivityScanner)];
Streak::StreakModel*     sModel   = nullptr;
Streak::ActivityScanner* sScanner = nullptr;
Streak::Found            sFound[kMaxFound];
char                     sScratch[Streak::StateCodec::kMaxBytes];
} // namespace

Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel)
{
    sModel   = new (sModelStorage) Streak::StreakModel();
    sScanner = new (sScannerStorage) Streak::ActivityScanner();
}

// -- The clock --------------------------------------------------------------------------

Service::Now Service::now() const
{
    Now n;
    const std::time_t utc = std::time(nullptr);
    std::tm           local {};
    localtime_r(&utc, &local);
    n.clockOk     = static_cast<int64_t>(utc) >= skClockFloorUtc;
    n.day         = Streak::WeekMath::daysFromCivil(local.tm_year + 1900, static_cast<uint32_t>(local.tm_mon + 1),
                                                    static_cast<uint32_t>(local.tm_mday));
    n.secondOfDay = static_cast<uint32_t>(local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec);
    return n;
}

// -- Settings -------------------------------------------------------------------------------

Streak::Goal Service::goalFromConfig() const
{
    Streak::Goal g;
    if (!mConfig) {
        return g;
    }
    const int32_t counts = mConfig->getInt(StreakConfig::kCounts);
    g.target     = static_cast<uint8_t>(mConfig->getInt(StreakConfig::kWeeklyTarget));
    g.weekStart  = static_cast<uint8_t>(mConfig->getInt(StreakConfig::kWeekStart));
    g.scope      = counts <= StreakConfig::kCountsAny ? Streak::kScopeAny : static_cast<uint8_t>(counts - 1);
    g.minMinutes = static_cast<uint8_t>(mConfig->getInt(StreakConfig::kMinMinutes));
    g.onePerDay  = mConfig->getBool(StreakConfig::kOnePerDay);
    return g.sane();
}

void Service::goalToConfig(const Streak::Goal& goal)
{
    if (!mConfig) {
        return;
    }
    mConfig->setInt(StreakConfig::kWeeklyTarget, goal.target);
    mConfig->setInt(StreakConfig::kWeekStart, goal.weekStart);
    mConfig->setInt(StreakConfig::kCounts, goal.scope == Streak::kScopeAny ? StreakConfig::kCountsAny : goal.scope + 1);
    mConfig->setInt(StreakConfig::kMinMinutes, goal.minMinutes);
    mConfig->setBool(StreakConfig::kOnePerDay, goal.onePerDay);
    if (!mConfig->save()) {
        LOG_ERROR("Could not save the settings\n");
    }
}

// -- Open: scan, credit, judge, save ------------------------------------------------------

void Service::open()
{
    // AppConfig is read here rather than in the constructor: in the simulator
    // the logger does not exist yet at construction time (as HybridX Race).
    mConfig.reset(new SDK::AppConfig(mKernel, StreakConfig::kFileName, StreakConfig::kFields,
                                     StreakConfig::kFieldCount));
    const Streak::Goal wanted = goalFromConfig();

    const auto source = Streak::StateCodec::load(mKernel.fs, kStateFile, sModel->state(), sScratch, sizeof(sScratch));
    if (source == Streak::StateCodec::Source::None) {
        LOG_INFO("No saved streak: a fresh start\n");
        sModel->reset(wanted);
    } else if (source == Streak::StateCodec::Source::Backup) {
        LOG_WARNING("state.json unreadable: using the backup\n");
    }

    const Now n = now();
    mClockOk    = n.clockOk;
    if (!n.clockOk) {
        LOG_WARNING("The clock is not set: nothing is judged until it is\n");
        return;
    }

    int32_t from = 0, to = 0;
    sModel->scanWindow(n.day, from, to);
    const uint32_t startMs = mKernel.sys.getTimeMs();
    const size_t   found   = sScanner->scan(mKernel.fs, APP_NAME, from, to, *sModel, sFound, kMaxFound);
    const auto&    st      = sScanner->stats();
    LOG_INFO("Scan days %ld..%ld: %u apps, %u new, %u read, %u rejected, %u recording, %u deferred, %lu ms%s\n",
             static_cast<long>(from), static_cast<long>(to), st.apps, st.candidates, st.read, st.rejected, st.recording,
             st.deferred, static_cast<unsigned long>(mKernel.sys.getTimeMs() - startMs),
             st.listed ? "" : " (cannot list ..)");

    sModel->update(n.day, sFound, found, mMoments);

    // A goal changed on the phone applies like one changed on the watch.
    const auto&        s       = sModel->state();
    const Streak::Goal current = s.hasPending ? s.pending : s.goal;
    if (wanted != current) {
        LOG_INFO("Goal changed in the settings\n");
        sModel->setGoal(wanted, n.day, mMoments);
    }
    save();
}

void Service::save()
{
    if (!Streak::StateCodec::save(mKernel.fs, kStateFile, sModel->state(), sScratch, sizeof(sScratch))) {
        LOG_ERROR("Could not save state.json\n");
    }
    // The public copy, for the glance (and HybridX Race later, PLAN 9).
    if (!mKernel.fs.mkdir(kSharedDir)
        || !Streak::StateCodec::save(mKernel.fs, kPublicFile, sModel->state(), sScratch, sizeof(sScratch))) {
        LOG_WARNING("Could not save the public streak.json\n");
    }
}

// -- Views ----------------------------------------------------------------------------------

void Service::sendViews()
{
    const Streak::StreakModel& m = *sModel;
    const Streak::State&       s = m.state();
    const Now                  n = now();

    if (auto home = SDK::make_msg<CustomMessage::HomeView>(mKernel)) {
        home->view = m.view(n.day);
        if (!mClockOk) {
            home->view.flags |= Streak::HomeView::kClockUnset;
        }
        if (s.pendingMissed > 0) {
            home->view.flags |= Streak::HomeView::kDecisionPending;
        }
        home.send();
    }

    if (auto week = SDK::make_msg<CustomMessage::WeekList>(mKernel)) {
        week->count      = s.sessionCount < CustomMessage::WeekList::kMax ? s.sessionCount : CustomMessage::WeekList::kMax;
        week->overflow   = s.overflow;
        week->minMinutes = s.goal.minMinutes;
        week->scope      = s.goal.scope;
        for (uint8_t i = 0; i < week->count; ++i) {
            const Streak::Session& x    = s.sessions[i];
            CustomMessage::WeekItem& it = week->items[i];
            it.kind    = static_cast<uint8_t>(x.kind);
            it.status  = static_cast<uint8_t>(m.status(i));
            it.manual  = (x.flags & Streak::Session::kManual) ? 1 : 0;
            it.app     = x.app;
            it.minutes = x.minutes;
            it.weekday = Streak::WeekMath::weekdayOf(static_cast<int32_t>(x.localStart / 86400u));
            it.hour    = static_cast<uint8_t>((x.localStart % 86400u) / 3600u);
        }
        week.send();
    }

    if (auto apps = SDK::make_msg<CustomMessage::AppNames>(mKernel)) {
        for (uint8_t i = 0; i < CustomMessage::AppNames::kMax && i < Streak::State::kApps; ++i) {
            std::strncpy(apps->names[i], s.apps[i], CustomMessage::AppNames::kChars - 1);
        }
        apps.send();
    }

    if (auto t = SDK::make_msg<CustomMessage::Trophies>(mKernel)) {
        t->weeksAchieved = m.liveWeeks();
        t->lifetime      = m.liveLifetime();
        t->longest       = s.longest > m.liveStreak() ? s.longest : m.liveStreak();
        const uint8_t q  = m.qualifying();
        t->bestWeek      = (m.weekMet() && q > s.bestWeek) ? q : s.bestWeek;
        t->badges        = s.badges;
        const uint8_t n  = s.historyCount < CustomMessage::Trophies::kRecent ? s.historyCount
                                                                             : CustomMessage::Trophies::kRecent;
        for (uint8_t i = 0; i < n; ++i) {
            const uint8_t at = static_cast<uint8_t>((s.historyNext + Streak::State::kHistory - n + i) % Streak::State::kHistory);
            t->recent[i]     = static_cast<uint8_t>(s.history[at].outcome);
        }
        t->recentCount = n;
        t.send();
    }

    if (auto g = SDK::make_msg<CustomMessage::GoalView>(mKernel)) {
        g->goal       = s.goal;
        g->pending    = s.pending;
        g->hasPending = s.hasPending ? 1 : 0;
        g.send();
    }
}

void Service::sendMoments()
{
    if (mMoments.count == 0) {
        return;
    }
    if (auto m = SDK::make_msg<CustomMessage::Moments>(mKernel)) {
        m->events = mMoments;
        if (m.send()) {
            mMoments = Streak::Events {};
        }
    }
}

// -- The loop ----------------------------------------------------------------------------------

void Service::run()
{
    LOG_INFO("Started\n");
    open();

    const uint32_t startMs = mKernel.sys.getTimeMs();
    while (true) {
        SDK::MessageBase* msg = nullptr;
        if (mKernel.comm.getMessage(msg, skWaitMs)) {
            if (msg->getType() == SDK::MessageType::COMMAND_APP_STOP) {
                LOG_INFO("Stop requested\n");
                mKernel.comm.releaseMessage(msg);
                return;
            }
            handle(msg);
            mKernel.comm.releaseMessage(msg);
        }

        // The GUI is the only reason to be here. Unsigned subtraction keeps the
        // check right across the millisecond clock's wrap.
        if (!mGuiStarted && mKernel.sys.getTimeMs() - startMs >= skStartupGraceMs) {
            LOG_INFO("No GUI, nothing to do: exiting\n");
            return;
        }
    }
}

void Service::handle(SDK::MessageBase* msg)
{
    Streak::StreakModel& m       = *sModel;
    bool                 changed = false;
    const Now            n       = now();

    switch (msg->getType()) {
        case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
            mGuiStarted = true;
            sendViews();
            sendMoments();
            return;

        case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
            mGuiStarted = false;
            return;

        case CustomMessage::CELEBRATE:
            celebrate(static_cast<CustomMessage::Celebrate*>(msg)->moment);
            return;

        case CustomMessage::LOG_MANUAL: {
            const auto* c = static_cast<CustomMessage::LogManual*>(msg);
            if (mClockOk && c->kind < Streak::kKindCount) {
                changed = m.logManual(static_cast<Streak::Kind>(c->kind), c->yesterday != 0, n.day, n.secondOfDay,
                                      mMoments);
            }
            break;
        }

        case CustomMessage::WEEK_ACTION: {
            const auto* c = static_cast<CustomMessage::WeekAction*>(msg);
            changed = c->action == CustomMessage::WeekAction::Undo ? m.undo(c->index, mMoments)
                                                                   : m.toggleExclude(c->index, mMoments);
            break;
        }

        case CustomMessage::SHIELD_DECISION:
            m.decideShields(static_cast<CustomMessage::ShieldDecision*>(msg)->use != 0, mMoments);
            changed = true;
            break;

        case CustomMessage::SET_GOAL: {
            const Streak::Goal g = static_cast<CustomMessage::SetGoal*>(msg)->goal.sane();
            if (mClockOk) {
                m.setGoal(g, n.day, mMoments);
            }
            // Without a clock only the settings change; the next open with a
            // clock applies them (open() compares the two).
            goalToConfig(g);
            changed = true;
            break;
        }

        default:
            // Unknown types are released and ignored, never treated as a
            // clock (service-lifecycle.md 4.1).
            return;
    }

    if (changed) {
        save();
        sendViews();
        sendMoments();
    }
}

// -- Haptics ------------------------------------------------------------------------------------

void Service::celebrate(CustomMessage::Moment moment)
{
    // Effects from the DRV2605-style library the kernel exposes
    // (CommandMessages.hpp RequestVibroPlay::Effect). A tick for the everyday,
    // a double click for a week done, a double pulse for a summit.
    switch (moment) {
        case CustomMessage::Moment::SessionFound: {
            static constexpr Effect kFx[] = { Effect::SHARP_TICK_1_100 };
            vibrate(kFx, 1, 0);
            break;
        }
        case CustomMessage::Moment::StepUp: {
            static constexpr Effect kFx[] = { Effect::SHORT_DOUBLE_CLICK_STRONG_1_100 };
            vibrate(kFx, 1, 0);
            backlightOn(skBacklightMs);
            break;
        }
        case CustomMessage::Moment::Summit: {
            static constexpr Effect kFx[] = { Effect::PULSING_STRONG_1_100, Effect::PULSING_STRONG_1_100 };
            vibrate(kFx, 2, 250);
            backlightOn(skBacklightMs);
            break;
        }
        case CustomMessage::Moment::Shield: {
            static constexpr Effect kFx[] = { Effect::SOFT_BUMP_100 };
            vibrate(kFx, 1, 0);
            break;
        }
    }
}

void Service::vibrate(const Effect* effects, uint8_t count, uint16_t gapMs)
{
    // N effects need 2N-1 notes (effects and pauses share the note array).
    const uint8_t maxCount = (SDK::Message::RequestVibroPlay::skMaxNotes + 1u) / 2u;
    if (count > maxCount) {
        count = maxCount;
    }
    if (count == 0u) {
        return;
    }

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestVibroPlay>();
    if (!msg) {
        return;
    }
    uint8_t n = 0u;
    for (uint8_t i = 0u; i < count; ++i) {
        msg->notes[n].effect = static_cast<uint8_t>(effects[i]);
        msg->notes[n].pause  = 0;
        ++n;
        if (i + 1u < count) {
            msg->notes[n].effect = static_cast<uint8_t>(Effect::NO_EFFECT);
            msg->notes[n].pause  = gapMs;
            ++n;
        }
    }
    msg->notesCount = n;
    mKernel.comm.sendMessage(msg);
    mKernel.comm.releaseMessage(msg);
}

void Service::backlightOn(uint32_t timeoutMs)
{
    if (auto bl = SDK::make_msg<SDK::Message::RequestBacklightSet>(mKernel)) {
        bl->brightness       = 100;
        bl->autoOffTimeoutMs = timeoutMs;
        bl.send();
    }
}
