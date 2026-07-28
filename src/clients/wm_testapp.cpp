/*
 * wm-testapp: minimal application-window client. Standalone process that
 * creates an ivi_surface with an APP-range id and paints a solid colour +
 * label via cairo -> GLES2. The wm-daemon discovers it and places it on
 * layer APP below the status bar.
 *
 * Launch two with different ids/colours to exercise app-window placement
 * (and, later, focus switching between them).
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

static const int APP_WIDTH  = 1024;
static const int APP_HEIGHT = 584;  /* screen_h - statusbar_h */

static void parse_rgb(const char *s, double *r, double *g, double *b)
{
    unsigned int R = 0x40, G = 0x60, B = 0xa0;
    if (s) {
        if (!strcmp(s, "red"))       { R=0xd0; G=0x30; B=0x30; }
        else if (!strcmp(s, "green")) { R=0x30; G=0xb0; B=0x40; }
        else if (!strcmp(s, "blue"))  { R=0x30; G=0x50; B=0xd0; }
        else if (!strcmp(s, "yellow")){ R=0xc0; G=0xa0; B=0x20; }
        else if (sscanf(s, "%u,%u,%u", &R, &G, &B) < 3) {
            CVI_DEBUG("[wm-testapp] bad --color '{}', using default", s);
            R=0x40; G=0x60; B=0xa0;
        }
    }
    *r = R / 255.0; *g = G / 255.0; *b = B / 255.0;
}

class TestApp : public StandardProcessFramework
{
public:
    TestApp(const std::string &name, int32_t argc, const char *argv[])
        : StandardProcessFramework(name, argc, argv)
        , m_initialized(false)
        , m_sid(WM_APP_ID_BASE)
        , m_color(nullptr)
        , m_label(nullptr)
    {
        for (int i = 1; i < argc; ++i) {
            if (!strcmp(argv[i], "--id") && i + 1 < argc) {
                m_sid = (uint32_t)strtoul(argv[++i], nullptr, 0);
            } else if (!strcmp(argv[i], "--color") && i + 1 < argc) {
                m_color = argv[++i];
            } else if (!strcmp(argv[i], "--label") && i + 1 < argc) {
                m_label = argv[++i];
            }
        }
    }

    ~TestApp() override = default;

    cvi::expected<bool, std::string> onInitialize() override
    {
        CVI_DEBUG("[wm-testapp] onInitialize: creating window id={}", m_sid);

        if (!m_win.create(m_sid, APP_WIDTH, APP_HEIGHT)) {
            return cvi::make_unexpected("WmWindow::create failed");
        }

        double r, g, b;
        parse_rgb(m_color, &r, &g, &b);

        cairo_t *cr = m_win.cairo_begin();
        if (!cr) {
            return cvi::make_unexpected("cairo_begin failed");
        }

        /* Solid colour background */
        cairo_set_source_rgb(cr, r, g, b);
        cairo_paint(cr);

        /* Centered label */
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 48);
        char text[128];
        if (m_label)
            snprintf(text, sizeof(text), "%s  (id %u)", m_label, m_sid);
        else
            snprintf(text, sizeof(text), "app id %u", m_sid);
        cairo_text_extents_t ext;
        cairo_text_extents(cr, text, &ext);
        double tx = (APP_WIDTH  - ext.width)  / 2.0 - ext.x_bearing;
        double ty = (APP_HEIGHT - ext.height) / 2.0 - ext.y_bearing;
        cairo_move_to(cr, tx, ty);
        cairo_show_text(cr, text);

        m_win.cairo_end();
        cairo_destroy(cr);
        m_win.draw(1.0f);

        m_initialized = true;
        CVI_DEBUG("[wm-testapp] running id={} {}x{}", m_sid, APP_WIDTH, APP_HEIGHT);
        return true;
    }

    cvi::expected<EXPECT_STATE, std::string> onRun() override
    {
        CVI_DEBUG("[wm-testapp] onRun: entering wayland event loop");
        m_win.run_loop(nullptr);
        return E_RUN_FINISH;
    }

    cvi::expected<bool, std::string> onSignal(int signum) override
    {
        CVI_DEBUG("[wm-testapp] onSignal({}): requesting exit", signum);
        if (m_initialized)
            m_win.request_exit();
        return true;
    }

    cvi::expected<bool, std::string> onDeinitialize() override
    {
        CVI_DEBUG("[wm-testapp] onDeinitialize: destroying window");
        m_win.destroy();
        m_initialized = false;
        return true;
    }

private:
    WmWindow     m_win;
    bool         m_initialized;
    uint32_t     m_sid;
    const char  *m_color;
    const char  *m_label;
};

/* ---- framework entry point ---- */

std::shared_ptr<StandardProcessFramework> pStandardProcessFramework = nullptr;

std::shared_ptr<StandardProcessFramework>
createStandardProcessFramework(int32_t argc, const char *argv[])
{
    if (!pStandardProcessFramework) {
        pStandardProcessFramework = std::make_shared<TestApp>(
            "wm-testapp", argc, argv);
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
