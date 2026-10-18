/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side link of Trail Quest to its service: sends what the player
 *          did, keeps the latest snapshots the service sent back.
 *
 * The game runs in the service (TrailQuest::Session). The GUI never changes
 * the game itself: it sends a command and draws the World (and Duel) that
 * answer it. Each command is numbered; the World echoes the number, so
 * waiting() says whether an answer is still to come.
 ******************************************************************************
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include "Commands.hpp"

class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    /// The screen that receives model events. Exactly one is bound at a time.
    void bind(ModelListener* listener) { modelListener = listener; }

    /// Exit the application: the kernel stops both processes.
    void exitApp();

    // -- What the service last sent ------------------------------------------
    bool                         hasWorld() const { return mHasWorld; }
    const TrailQuest::WorldView& world() const { return mWorld; }
    const TrailQuest::DuelView&  duel() const { return mDuel; }
    bool                         beatOn() const { return mBeat; }

    /// A command is out and its World has not come back yet.
    bool waiting() const { return mAck != mSeq; }

    /// The number of the command the last World answered.
    uint16_t ack() const { return mAck; }

    // -- Commands (each answered by a World) ----------------------------------
    void hello();
    void newGame();
    void travel(uint8_t exitIndex);
    void obstacleTry(TrailQuest::BeatResult beat);
    void duelTurn(TrailQuest::Action action, TrailQuest::ItemId item, TrailQuest::BeatResult beat);
    void join(bool accept, uint8_t slot);
    void advance();
    void setBeat(bool on);

    /// Ask the service to play a feedback pattern (no answer).
    void playFx(TrailQuest::FxId id);

    /// Call once a frame. If an answer is overdue (a message lost to a full
    /// queue or pool), ask for the world again: its answer settles waiting().
    void keepAlive(uint32_t nowMs);

protected:
    ModelListener*     modelListener;   ///< The screen on display
    const SDK::Kernel& mKernel;         ///< The GUI process's kernel interface

    // IGuiLifeCycleCallback
    void onStart() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;

    // ICustomMessageHandler: messages from the service
    bool customMessageHandler(SDK::MessageBase* msg) override;

private:
    /// How long an answer may take before keepAlive() asks again, ms (a round
    /// trip is one frame, 100 ms, on the watch: Gate Q0).
    static constexpr uint32_t kAnswerMs = 2000;

    uint16_t next();

    TrailQuest::WorldView mWorld {};
    TrailQuest::DuelView  mDuel {};
    bool                  mHasWorld = false;
    bool                  mBeat     = true;
    uint16_t              mSeq      = 0;
    uint16_t              mAck      = 0;
    uint32_t              mSentAt   = 0;
};

#endif // MODEL_HPP
