/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the screen on display.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

/// A screen keeps its own reference to the Model and reads the snapshots from
/// it; these only say that something arrived.
class ModelListener
{
public:
    virtual ~ModelListener() = default;

    /// A new World snapshot is in the Model (after every command, and at start).
    virtual void onWorld() {}

    /// A new Duel snapshot is in the Model (always before the World that
    /// answers the same command).
    virtual void onDuel() {}

    /// The GUI is going off screen: a hold in progress will never see its release.
    virtual void onSuspend() {}
};

#endif // MODEL_LISTENER_HPP
