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

    /// The service judged a timing-beat press.
    virtual void onBeatResult(const CustomMessage::BeatResult& r) { (void)r; }

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
