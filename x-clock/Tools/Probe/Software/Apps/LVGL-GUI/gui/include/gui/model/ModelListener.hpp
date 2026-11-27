/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   What the Clock Probe's screen hears from its model.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

class ModelListener
{
public:
    virtual ~ModelListener() = default;

    /// A new view or a new time from the service.
    virtual void onChanged() {}
};

#endif // MODEL_LISTENER_HPP
