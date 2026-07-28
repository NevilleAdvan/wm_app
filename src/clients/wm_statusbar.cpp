/*
 * wm-statusbar: status bar chrome client. Standalone process. Creates
 * ivi_surface WM_SID_STATUS, paints a grey bar with three icons loaded
 * from res/statusbar/{100,101,102}.png via cairo, uploads to GLES2. The
 * daemon places it on layer STATUS.
 *
 * Phase 3-0-B: migrated to StandardProcessFramework + WmWindow. */
#include "wm/WmWindow.h"
#include "wm/surface_ids.h"
#include "cvi-lib-servicefw/StandardProcessFramework.h"

#include <cairo/cairo.h>
#include <cvi/log/log_service.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <vector>

#define STATUSBAR_ICON_DIRS "res/statusbar"
#define STATUSBAR_ICON_SIZE 40
#define STATUSBAR_PAD 8
#define STATUSBAR_ICONS 3

static const int SB_WIDTH  = 1024;
static const int SB_HEIGHT = WM_STATUSBAR_H;

static const char *const k_icons[STATUSBAR_ICONS] = {
    STATUSBAR_ICON_DIRS "/100.png",
    STATUSBAR_ICON_DIRS "/101.png",
    STATUSBAR_ICON_DIRS "/102.png",
};

class StatusbarApp : public StandardProcessFramework
{
public:
    StatusbarApp(const std::string &name, int32_t argc, const char *argv[])
        : StandardProcessFramework(name, argc, argv)
        , m_initialized(false)
    {
        (void)argc; (void)argv;
    }

    ~StatusbarApp() override = default;

    cvi::expected<bool, std::string> onInitialize() override
    {
        CVI_DEBUG("[wm-statusbar] onInitialize: creating window id={}", WM_SID_STATUS);

        if (!m_win.create(WM_SID_STATUS, SB_WIDTH, SB_HEIGHT)) {
            return cvi::make_unexpected("WmWindow::create failed");
        }

        /* Paint once: grey bar + icons */
        cairo_t *cr = m_win.cairo_begin();
        if (!cr) {
            return cvi::make_unexpected("cairo_begin failed");
        }

        /* Grey background */
        double r = ((WM_STATUSBAR_BG >> 16) & 0xff) / 255.0;
        double g = ((WM_STATUSBAR_BG >> 8) & 0xff) / 255.0;
        double b = ((WM_STATUSBAR_BG) & 0xff) / 255.0;
        cairo_set_source_rgb(cr, r, g, b);
        cairo_paint(cr);

        /* Icons left-aligned, vertically centered */
        double icon_y = ((double)SB_HEIGHT - STATUSBAR_ICON_SIZE) / 2.0;
        double x = STATUSBAR_PAD;
        for (int i = 0; i < STATUSBAR_ICONS; ++i) {
            cairo_surface_t *ico = cairo_image_surface_create_from_png(k_icons[i]);
            if (cairo_surface_status(ico) == CAIRO_STATUS_SUCCESS) {
                cairo_set_source_surface(cr, ico, x, icon_y);
                cairo_paint(cr);
            } else {
                CVI_DEBUG("[wm-statusbar] load {} failed", k_icons[i]);
            }
            cairo_surface_destroy(ico);
            x += STATUSBAR_ICON_SIZE + STATUSBAR_PAD;
        }

        m_win.cairo_end();
        cairo_destroy(cr);

        /* Upload + first draw */
        m_win.draw(1.0f);

        m_initialized = true;
        CVI_DEBUG("[wm-statusbar] running id={} {}x{}", WM_SID_STATUS, SB_WIDTH, SB_HEIGHT);
        return true;
    }

    cvi::expected<EXPECT_STATE, std::string> onRun() override
    {
        CVI_DEBUG("[wm-statusbar] onRun: entering wayland event loop");
        m_win.run_loop(nullptr);
        return E_RUN_FINISH;
    }

    cvi::expected<bool, std::string> onSignal(int signum) override
    {
        CVI_DEBUG("[wm-statusbar] onSignal({}): requesting exit", signum);
        if (m_initialized)
            m_win.request_exit();
        return true;
    }

    cvi::expected<bool, std::string> onDeinitialize() override
    {
        CVI_DEBUG("[wm-statusbar] onDeinitialize: destroying window");
        m_win.destroy();
        m_initialized = false;
        return true;
    }

private:
    WmWindow  m_win;
    bool      m_initialized;
};

/* ---- framework entry point ---- */

std::shared_ptr<StandardProcessFramework> pStandardProcessFramework = nullptr;

std::shared_ptr<StandardProcessFramework>
createStandardProcessFramework(int32_t argc, const char *argv[])
{
    if (!pStandardProcessFramework) {
        pStandardProcessFramework = std::make_shared<StatusbarApp>(
            "wm-statusbar", argc, argv);
    }
    return pStandardProcessFramework;
}

int32_t main(int32_t argc, const char *argv[])
{
    auto p = createStandardProcessFramework(argc, argv);
    if (p) {
        p->launch();
    } else {
        fprintf(stderr, "createStandardProcessFramework failed\n");
        return 1;
    }
    return 0;
}
