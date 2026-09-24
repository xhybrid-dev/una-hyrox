/**
 ******************************************************************************
 * @file    ProbeGui.cpp
 * @brief   The Streak Probe's one screen: the verdict and the numbers behind it.
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

constexpr int kRows = 5;

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

        Draw::label(mRoot, &poppins_regular_14, "Streak Probe", 40, 16, 160, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        mVerdict = Draw::label(mRoot, &poppins_semibold_25, "Probing...", 20, 38, 200, LV_TEXT_ALIGN_CENTER,
                               Color::WHITE);
        mMeaning = Draw::label(mRoot, &poppins_regular_14, "Looking at other apps' files", 30, 70, 180,
                               LV_TEXT_ALIGN_CENTER, Color::GRAY);
        lv_label_set_long_mode(mMeaning, LV_LABEL_LONG_WRAP);
        for (int i = 0; i < kRows; ++i) {
            mRow[i] = Draw::label(mRoot, &poppins_regular_16, "", 24, 108 + i * 19, 192, LV_TEXT_ALIGN_CENTER,
                                  Color::WHITE);
        }
        mFooter = Draw::label(mRoot, &poppins_regular_14, "R2 to exit", 50, 204, 140, LV_TEXT_ALIGN_CENTER,
                              Color::GRAY);
        lv_screen_load(mRoot);
    }

    void show(const Probe::Result& r)
    {
        const char* meaning = "";
        uint32_t    colour  = Color::YELLOW_DARK;
        switch (r.verdict) {
            case Verdict::Go:
                meaning = "Other apps' activities can be read";
                colour  = Color::LIME;
                break;
            case Verdict::NoFiles:
                meaning = "Record any activity, then run again";
                break;
            case Verdict::NoOpen:
                meaning = "Files are listed but cannot be read";
                break;
            case Verdict::Blocked:
                meaning = "The watch keeps apps' files apart";
                break;
            case Verdict::Running:
                colour = Color::WHITE;
                break;
        }
        lv_label_set_text(mVerdict, Probe::verdictName(r.verdict));
        lv_obj_set_style_text_color(mVerdict, Draw::rgb(colour), LV_PART_MAIN);
        lv_label_set_text(mMeaning, meaning);

        char text[48];
        snprintf(text, sizeof(text), "Apps %u, with files %u", static_cast<unsigned>(r.apps),
                 static_cast<unsigned>(r.appsWithFit));
        lv_label_set_text(mRow[0], text);
        snprintf(text, sizeof(text), "Activity files %u", static_cast<unsigned>(r.fitFiles + r.otherFit));
        lv_label_set_text(mRow[1], text);
        snprintf(text, sizeof(text), "Read %u KB, %u ms", static_cast<unsigned>(r.readBytes / 1024u),
                 static_cast<unsigned>(r.readMs));
        lv_label_set_text(mRow[2], text);
        snprintf(text, sizeof(text), "Shared folder %s", Probe::checkName(r.sharedData));
        lv_label_set_text(mRow[3], text);
        if (r.glance == Check::Ok) {
            snprintf(text, sizeof(text), "Glance %dx%d, %u", r.glanceWidth, r.glanceHeight,
                     static_cast<unsigned>(r.glanceControls));
        } else {
            snprintf(text, sizeof(text), "Glance: no reply");
        }
        lv_label_set_text(mRow[4], text);

        snprintf(text, sizeof(text), "Run %u saved", static_cast<unsigned>(r.run));
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
    lv_obj_t*    mRow[kRows] {};
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
