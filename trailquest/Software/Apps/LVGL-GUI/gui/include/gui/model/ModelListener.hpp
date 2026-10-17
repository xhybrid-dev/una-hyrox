/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

#include "Commands.hpp"

/// A screen keeps its own reference to the Model; this only carries events.
class ModelListener
{
public:
    virtual ~ModelListener() = default;

    /// The service judged a timing-beat press.
    virtual void onBeatResult(const CustomMessage::BeatResult& r) { (void)r; }
};

#endif // MODEL_LISTENER_HPP
