/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   What the bike probe's screen hears from its model.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

class Model;

class ModelListener
{
public:
    virtual ~ModelListener() = default;

    /// A new line in the log, or a new state.
    virtual void onLog() {}
    virtual void onState() {}
};

#endif // MODEL_LISTENER_HPP
