/**
 ******************************************************************************
 * @file    ProbeGui.cpp
 * @brief   The Trail Probe's one screen: the route, GPS, compass and memory.
 *
 * The service does all the work and sends a ProbeResult once a second while
 * it samples the sensors. Until the first arrives the screen says
 * "Probing...". R2 exits. The full report is in probe.txt; this screen is for
 * a photo.
 ******************************************************************************
 */

#include <cstdio>
#include <new>

#include "lvgl.h"

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

#include "ProbeFonts.hpp"
#include "ProbeMessages.hpp"

namespace
{

namespace Draw  = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;
using Probe::Sense;
using Probe::Verdict;

class ProbeScreen : public SDK::Interface::IGuiLifeCycleCallback,
                    public SDK::Interface::ICustomMessageHandler
{
public:
    ProbeScreen()
        : mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
    {
        SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
        SDK::LVGL::Port::GetInstance().setCustomMessageHandler(this);
        build();
    }

    bool customMessageHandler(SDK::MessageBase* msg) override
    {
        if (msg->getType() == CustomMessage::PROBE_RESULT) {
            show(static_cast<CustomMessage::ProbeResult*>(msg)->result);
        }
        return true;   // the port releases every custom message itself
    }

private:
    void build()
    {
        mRoot = lv_obj_create(nullptr);
        Draw::applyScreen(mRoot);
        lv_obj_add_event_cb(mRoot, &ProbeScreen::onKey, LV_EVENT_KEY, this);

        Draw::label(mRoot, &poppins_regular_14, "Trail Probe", 30, 14, 180, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        mVerdict = Draw::label(mRoot, &poppins_semibold_25, "Probing...", 20, 34, 200, LV_TEXT_ALIGN_CENTER,
                               Color::WHITE);
        mRoute   = Draw::label(mRoot, &poppins_regular_14, "Looking in Routes/", 24, 66, 192, LV_TEXT_ALIGN_CENTER,
                               Color::GRAY);
        mDetail  = Draw::label(mRoot, &poppins_regular_14, "", 20, 86, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        mGps     = Draw::label(mRoot, &poppins_regular_16, "", 16, 112, 208, LV_TEXT_ALIGN_CENTER, Color::WHITE);
        mCompass = Draw::label(mRoot, &poppins_regular_16, "", 16, 136, 208, LV_TEXT_ALIGN_CENTER, Color::WHITE);
        mOff     = Draw::label(mRoot, &poppins_regular_16, "", 16, 160, 208, LV_TEXT_ALIGN_CENTER, Color::WHITE);
        mMemory  = Draw::label(mRoot, &poppins_regular_14, "", 30, 186, 180, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        mFooter  = Draw::label(mRoot, &poppins_regular_14, "R2 to exit", 50, 206, 140, LV_TEXT_ALIGN_CENTER,
                               Color::GRAY);
        lv_screen_load(mRoot);
    }

    void show(const Probe::Result& r)
    {
        char text[64];

        uint32_t colour = Color::YELLOW_DARK;
        switch (r.verdict) {
            case Verdict::Go:           colour = Color::LIME; break;
            case Verdict::FolderFailed: colour = Color::RED; break;
            case Verdict::Running:      colour = Color::WHITE; break;
            default:                    break;
        }
        lv_label_set_text(mVerdict, Probe::verdictName(r.verdict));
        lv_obj_set_style_text_color(mVerdict, Draw::rgb(colour), LV_PART_MAIN);

        switch (r.verdict) {
            case Verdict::Go:
            case Verdict::Unreadable:
                snprintf(text, sizeof(text), "%.34s", r.name[0] ? r.name : r.file);
                break;
            case Verdict::NoRoute:
                snprintf(text, sizeof(text), "Copy a .gpx into Routes/");
                break;
            case Verdict::FolderFailed:
                snprintf(text, sizeof(text), "Could not open Routes/");
                break;
            case Verdict::Running:
                text[0] = '\0';
                break;
        }
        lv_label_set_text(mRoute, text);

        if (r.verdict == Verdict::Go) {
            snprintf(text, sizeof(text), "%lu.%lu km, %u pts, %lu ms", static_cast<unsigned long>(r.lengthM / 1000u),
                     static_cast<unsigned long>((r.lengthM % 1000u) / 100u), static_cast<unsigned>(r.kept),
                     static_cast<unsigned long>(r.readMs));
        } else if (r.gpxCount > 0) {
            snprintf(text, sizeof(text), "%u .gpx, %lu points read", static_cast<unsigned>(r.gpxCount),
                     static_cast<unsigned long>(r.rawPoints));
        } else {
            text[0] = '\0';
        }
        lv_label_set_text(mDetail, text);

        switch (r.gps) {
            case Sense::Ok:
                snprintf(text, sizeof(text), "GPS: fix in %u s, %u m", static_cast<unsigned>(r.fixAfterS),
                         static_cast<unsigned>((r.precisionDm + 5u) / 10u));
                break;
            case Sense::Searching:
            case Sense::NoData:
                snprintf(text, sizeof(text), "GPS: %s %u s", Probe::senseName(r.gps),
                         static_cast<unsigned>(r.elapsedS));
                break;
            default:
                snprintf(text, sizeof(text), "GPS: %s", Probe::senseName(r.gps));
                break;
        }
        lv_label_set_text(mGps, text);

        if (r.compass == Sense::Ok) {
            if (r.tiltedDeg >= 0) {
                snprintf(text, sizeof(text), "Compass: %d deg (level %d)", static_cast<int>(r.tiltedDeg),
                         static_cast<int>(r.bearingDeg));
            } else {
                snprintf(text, sizeof(text), "Compass: level %d deg", static_cast<int>(r.bearingDeg));
            }
        } else if (r.compass == Sense::Searching) {
            snprintf(text, sizeof(text), "Compass: not calibrated");
        } else {
            snprintf(text, sizeof(text), "Compass: %s", Probe::senseName(r.compass));
        }
        lv_label_set_text(mCompass, text);

        if (r.offRouteM >= 10000) {
            snprintf(text, sizeof(text), "Off route: %ld km", static_cast<long>(r.offRouteM / 1000));
        } else if (r.offRouteM >= 0) {
            snprintf(text, sizeof(text), "Off route: %ld m", static_cast<long>(r.offRouteM));
        } else {
            snprintf(text, sizeof(text), "Off route: -");
        }
        lv_label_set_text(mOff, text);

        snprintf(text, sizeof(text), "Largest block %lu KB", static_cast<unsigned long>(r.largestAllocB / 1024u));
        lv_label_set_text(mMemory, text);

        if (r.finished) {
            snprintf(text, sizeof(text), "Run %u saved - R2 exit", static_cast<unsigned>(r.run));
        } else {
            snprintf(text, sizeof(text), "Run %u, %u s - R2 exit", static_cast<unsigned>(r.run),
                     static_cast<unsigned>(r.elapsedS));
        }
        lv_label_set_text(mFooter, text);
    }

    static void onKey(lv_event_t* e)
    {
        auto* self = static_cast<ProbeScreen*>(lv_event_get_user_data(e));
        if (static_cast<uint8_t>(lv_event_get_key(e)) == SDK::GUI::Button::R2) {
            SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
            SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
            self->mKernel.sys.exit();
        }
    }

    const SDK::Kernel& mKernel;
    lv_obj_t*    mRoot    = nullptr;
    lv_obj_t*    mVerdict = nullptr;
    lv_obj_t*    mRoute   = nullptr;
    lv_obj_t*    mDetail  = nullptr;
    lv_obj_t*    mGps     = nullptr;
    lv_obj_t*    mCompass = nullptr;
    lv_obj_t*    mOff     = nullptr;
    lv_obj_t*    mMemory  = nullptr;
    lv_obj_t*    mFooter  = nullptr;
};

alignas(ProbeScreen) uint8_t sScreenStorage[sizeof(ProbeScreen)];

} // namespace

extern "C" const lv_font_t* una_lvgl_default_font(void)
{
    return &poppins_regular_16;
}

extern "C" void una_lvgl_app_init(void)
{
    Draw::init();
    new (sScreenStorage) ProbeScreen();
}
