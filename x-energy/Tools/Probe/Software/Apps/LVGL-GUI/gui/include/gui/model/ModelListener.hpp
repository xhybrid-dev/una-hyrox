/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   What the Energy Probe's screen hears from its model.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

class ModelListener
{
public:
    virtual ~ModelListener() = default;

    /// New status, nights or check results from the service.
    virtual void onChanged() {}
};

#endif // MODEL_LISTENER_HPP
