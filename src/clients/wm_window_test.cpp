/*
 * wm-window-test: StandardProcessFramework-based test app for libwm_window.
 * Verifies that WmWindow::create() produces an ivi_surface that the
 * wm-transition compositor module can discover (on_surface_configured).
 *
 * Expected behaviour:
 *   1. Process launches, StandardProcessFramework calls onPrepare→onInitialize
 *   2. onInitialize creates a WmWindow with a test ivi id (WM_APP_ID_BASE+99)
 *   3. onRun enters WmWindow::run_loop (wayland event loop with self-pipe)
 *   4. wm-transition.so calls on_surface_configured → classifies into APP layer
 *   5. On SIGTERM/SIGINT, onSignal → request_exit wakes the loop
 *   6. onDeinitialize destroys the window
 *
 * Run with: wm-window-test
 * Check with: journalctl (look for "wm-transition: surface 1299 configured")
 *   or: wm-ctl list (should show ivi_id 1299 in the app layer)
 */

#include "wm/WmWindow.h"
#include "wm/surface_ids.h"
#include "cvi-lib-servicefw/StandardProcessFramework.h"

#include <cairo/cairo.h>
#include <cvi/log/log_service.h>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

static const int TEST_WIDTH  = 1280;
static const int TEST_HEIGHT = 720;
static const uint32_t TEST_SID = WM_APP_ID_BASE + 99;  /* 1299 */

class WindowTestApp : public StandardProcessFramework
{
public:
    WindowTestApp(const std::string &name, int32_t argc, const char *argv[])
        : StandardProcessFramework(name, argc, argv)
        , m_initialized(false)
    {
    }

    ~WindowTestApp() override = default;

    cvi::expected<bool, std::string> onInitialize() override
    {
        CVI_DEBUG("[wm-test] onInitialize: creating window id={} {}x{}",
                  TEST_SID, TEST_WIDTH, TEST_HEIGHT);

        bool ok = m_win.create(TEST_SID, TEST_WIDTH, TEST_HEIGHT);
        if (!ok) {
            CVI_DEBUG("[wm-test] WmWindow::create failed");
            return cvi::make_unexpected("WmWindow::create failed");
        }

        CVI_DEBUG("[wm-test] WmWindow::create succeeded");
        m_initialized = true;
        return true;
    }

    cvi::expected<EXPECT_STATE, std::string> onRun() override
    {
        CVI_DEBUG("[wm-test] onRun: entering wayland event loop");

        /* Enter the wayland event loop (blocks until request_exit).
         * Returns E_RUN_FINISH so launch() skips startServiceLoop(). */
        m_win.run_loop([this](cairo_t *cr, int /*w*/, int /*h*/) {
            /* Dark teal background */
            cairo_set_source_rgb(cr, 0.05, 0.20, 0.25);
            cairo_paint(cr);

            /* Centered white label */
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 36);
            const char *text = "wm-window-test: OK";
            cairo_text_extents_t ext;
            cairo_text_extents(cr, text, &ext);
            cairo_move_to(cr,
                (TEST_WIDTH  - ext.width)  / 2.0 - ext.x_bearing,
                (TEST_HEIGHT - ext.height) / 2.0 - ext.y_bearing);
            cairo_show_text(cr, text);
        });

        CVI_DEBUG("[wm-test] onRun: loop exited");
        return E_RUN_FINISH;
    }

    cvi::expected<bool, std::string> onSignal(int signum) override
    {
        CVI_DEBUG("[wm-test] onSignal({}): requesting exit", signum);
        if (m_initialized) {
            m_win.request_exit();
        }
        return true;
    }

    cvi::expected<bool, std::string> onDeinitialize() override
    {
        CVI_DEBUG("[wm-test] onDeinitialize: destroying window");
        m_win.destroy();
        m_initialized = false;
        return true;
    }

private:
    WmWindow m_win;
    bool     m_initialized;
};

/* ---- framework entry point ---- */

std::shared_ptr<StandardProcessFramework> pStandardProcessFramework = nullptr;

std::shared_ptr<StandardProcessFramework>
createStandardProcessFramework(int32_t argc, const char *argv[])
{
    if (!pStandardProcessFramework) {
        pStandardProcessFramework = std::make_shared<WindowTestApp>(
            "wm-window-test", argc, argv);
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
