/**
 ******************************************************************************
 * @file    ProbeGui.cpp
 * @brief   The Intervals Probe's one screen: did a plan file arrive?
 *
 * The service does all the work and sends one ProbeResult when this GUI
 * starts. Until it arrives the screen says "Probing...". R2 exits. The full
 * report is in probe.txt; this screen is for a photo.
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
using Probe::Check;
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

        Draw::label(mRoot, &poppins_regular_14, "Intervals Probe", 30, 16, 180, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        mVerdict = Draw::label(mRoot, &poppins_semibold_25, "Probing...", 20, 40, 200, LV_TEXT_ALIGN_CENTER,
                               Color::WHITE);
        mMeaning = Draw::label(mRoot, &poppins_regular_14, "Looking in Plans/", 30, 72, 180, LV_TEXT_ALIGN_CENTER,
                               Color::GRAY);
        mCount   = Draw::label(mRoot, &poppins_regular_16, "", 24, 106, 192, LV_TEXT_ALIGN_CENTER, Color::WHITE);
        mNewest  = Draw::label(mRoot, &poppins_regular_14, "", 24, 130, 192, LV_TEXT_ALIGN_CENTER, Color::WHITE);
        mPreview = Draw::label(mRoot, &poppins_regular_14, "", 30, 154, 180, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        lv_label_set_long_mode(mPreview, LV_LABEL_LONG_WRAP);
        mFooter  = Draw::label(mRoot, &poppins_regular_14, "R2 to exit", 50, 204, 140, LV_TEXT_ALIGN_CENTER,
                               Color::GRAY);
        lv_screen_load(mRoot);
    }

    void show(const Probe::Result& r)
    {
        const char* meaning = "";
        uint32_t    colour  = Color::YELLOW_DARK;
        switch (r.verdict) {
            case Verdict::Go:
                meaning = "A file arrived and was read";
                colour  = Color::LIME;
                break;
            case Verdict::Empty:
                meaning = "No file yet -- send one and run again";
                break;
            case Verdict::FolderFailed:
                meaning = "Could not create/open Plans/";
                colour  = Color::RED;
                break;
            case Verdict::Running:
                colour = Color::WHITE;
                break;
        }
        lv_label_set_text(mVerdict, Probe::verdictName(r.verdict));
        lv_obj_set_style_text_color(mVerdict, Draw::rgb(colour), LV_PART_MAIN);
        lv_label_set_text(mMeaning, meaning);

        char text[64];
        snprintf(text, sizeof(text), "%u file(s) in Plans/", static_cast<unsigned>(r.fileCount));
        lv_label_set_text(mCount, text);
        if (r.newestName[0]) {
            snprintf(text, sizeof(text), "%.40s, %u B", r.newestName, static_cast<unsigned>(r.newestSize));
        } else {
            text[0] = '\0';
        }
        lv_label_set_text(mNewest, text);
        lv_label_set_text(mPreview, r.preview);

        snprintf(text, sizeof(text), "Run %u saved -- R2 to exit", static_cast<unsigned>(r.run));
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
    lv_obj_t*    mMeaning = nullptr;
    lv_obj_t*    mCount   = nullptr;
    lv_obj_t*    mNewest  = nullptr;
    lv_obj_t*    mPreview = nullptr;
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
