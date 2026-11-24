/**
 ******************************************************************************
 * @file    AddRoutesScreen.hpp
 * @brief   HybridX Trail: where GPX files go, from the route list's "Add
 *          routes" (R1). R1 or R2 goes back to the list.
 *
 * The folder is this app's own in Apps/, as the service worked it out
 * (Service::findAppFolder): the portal App ID for a store install
 * (1C83FDDE4EA3C880), HybridXTrail for a development copy. T4: a store
 * install's folder name means nothing to a runner, so the watch says it.
 ******************************************************************************
 */

#ifndef ADD_ROUTES_SCREEN_HPP
#define ADD_ROUTES_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class AddRoutesScreen : public Screen
{
public:
    explicit AddRoutesScreen(Model& model);

    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // ADD_ROUTES_SCREEN_HPP
