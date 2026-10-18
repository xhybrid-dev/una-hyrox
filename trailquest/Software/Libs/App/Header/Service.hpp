/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   Trail Quest background logic: runs the game (TrailQuest::Session),
 *          answers the GUI with snapshots, keeps the save file and the
 *          settings, and plays the feedback patterns.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "Commands.hpp"
#include "SaveStore.hpp"
#include "Session.hpp"

#include "SDK/Kernel/KernelProviderService.hpp"

#include <cstdint>

class Service
{
public:
    explicit Service(SDK::Kernel &kernel);

    virtual ~Service() = default;

    void run();

private:
    /// Ask the kernel to keep the backlight on for this long. A game is looked
    /// at between button presses, so the default timeout would turn it off.
    void holdBacklight(uint32_t timeoutMs);

    /// Vibro and buzzer for one TrailQuest::FxId.
    void playFx(TrailQuest::FxId id);

    /// Load the save and the settings, and start the session.
    void startUp();

    /// Handle one command from the GUI. True if it was one of ours.
    bool onCommand(const SDK::MessageBase &msg);

    /// The world (and the duel, if there is one to show) to the GUI.
    void sendViews();

    /// Save if the game asks to (after the views are sent, so the screen never
    /// waits on the file system).
    void saveIfAsked();

    /// A seed or a stir: the kernel's clock, so each start differs. The
    /// simulator can fix it (TRAILQUEST_SEED) for repeatable captures.
    uint32_t entropy() const;

    SDK::Kernel &                    mKernel;
    TrailQuest::Session              mSession;
    TrailQuest::SaveStore::Settings  mSettings;
    uint16_t                         mAck = 0;
};

#endif // SERVICE_HPP
