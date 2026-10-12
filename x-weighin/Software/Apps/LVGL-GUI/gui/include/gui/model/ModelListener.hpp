/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

#include "Commands.hpp"

class Model;

class ModelListener
{
public:
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() = default;

    void bind(Model* m) { model = m; }

    /// A new first look arrived from the service.
    virtual void onFirstLook(const CustomMessage::FirstLookState& look) { (void)look; }

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
