/*
 * wm-bg: background chrome client. Standalone process so a crash does not
 * take down wm-daemon. Creates ivi_surface WM_SID_BG, paints res/bg.jpeg
 * (cover-scaled) via cairo, uploads to GLES2. The daemon discovers this
 * surface and places it on layer BG.
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
#include <jpeglib.h>

/* Decode baseline JPEG to cairo ARGB32 (LE B,G,R,A). malloc'd. */
static uint8_t *decode_jpeg_argb(const char *path, int *out_w, int *out_h)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, f);
    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&cinfo); fclose(f); return NULL;
    }
    jpeg_start_decompress(&cinfo);
    int w = (int)cinfo.output_width, h = (int)cinfo.output_height;
    int ch = (int)cinfo.output_components;
    if (ch != 3) {
        jpeg_destroy_decompress(&cinfo); fclose(f); return NULL;
    }
    uint8_t *argb = static_cast<uint8_t *>(malloc((size_t)w * h * 4));
    if (!argb) { jpeg_destroy_decompress(&cinfo); fclose(f); return NULL; }
    JSAMPARRAY rows = (*cinfo.mem->alloc_sarray)(
        (j_common_ptr)&cinfo, JPOOL_IMAGE, (JDIMENSION)(w * ch), 1);
    while (cinfo.output_scanline < (JDIMENSION)h) {
        unsigned int row = cinfo.output_scanline;
        jpeg_read_scanlines(&cinfo, rows, 1);
        uint8_t *d = argb + (size_t)row * w * 4;
        for (int x = 0; x < w; ++x) {
            d[x*4+0] = rows[0][x*3+2]; d[x*4+1] = rows[0][x*3+1];
            d[x*4+2] = rows[0][x*3+0]; d[x*4+3] = 0xff;
        }
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(f);
    *out_w = w; *out_h = h;
    return argb;
}

static const int BG_WIDTH  = 1024;
static const int BG_HEIGHT = 640;

class BgApp : public StandardProcessFramework
{
public:
    BgApp(const std::string &name, int32_t argc, const char *argv[])
        : StandardProcessFramework(name, argc, argv)
        , m_initialized(false)
        , m_bg_image("res/client_bg.jpeg")
    {
        for (int i = 1; i < argc; ++i) {
            if (!strcmp(argv[i], "--image") && i + 1 < argc)
                m_bg_image = argv[++i];
        }
    }

    ~BgApp() override = default;

    cvi::expected<bool, std::string> onInitialize() override
    {
        CVI_DEBUG("[wm-bg] onInitialize: creating window id={}", WM_SID_BG);

        /* Guess screen size from output. WmWindow doesn't query output
         * size yet; use fixed 1024x640 and accept ILM-driven placement. */
        if (!m_win.create(WM_SID_BG, BG_WIDTH, BG_HEIGHT)) {
            return cvi::make_unexpected("WmWindow::create failed");
        }

        /* Decode JPEG */
        int jw = 0, jh = 0;
        uint8_t *img = decode_jpeg_argb(m_bg_image.c_str(), &jw, &jh);
        if (!img) {
            CVI_DEBUG("[wm-bg] decode {} failed; solid fallback", m_bg_image);
        }

        /* Paint once: solid dark + cover-scaled JPEG */
        cairo_t *cr = m_win.cairo_begin();
        if (!cr) {
            free(img);
            return cvi::make_unexpected("cairo_begin failed");
        }
        cairo_set_source_rgb(cr, 0.03, 0.03, 0.05);
        cairo_paint(cr);

        if (img) {
            int sstride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, jw);
            cairo_surface_t *src = cairo_image_surface_create_for_data(
                img, CAIRO_FORMAT_ARGB32, jw, jh, sstride);
            if (cairo_surface_status(src) == CAIRO_STATUS_SUCCESS) {
                double sxr = (double)BG_WIDTH / jw, syr = (double)BG_HEIGHT / jh;
                double sc = sxr > syr ? sxr : syr;
                double dw = jw * sc, dh = jh * sc;
                cairo_translate(cr, ((double)BG_WIDTH - dw) * 0.5,
                                ((double)BG_HEIGHT - dh) * 0.5);
                cairo_scale(cr, sc, sc);
                cairo_set_source_surface(cr, src, 0, 0);
                cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
                cairo_paint(cr);
            }
            cairo_surface_destroy(src);
            free(img);
        }

        m_win.cairo_end();
        cairo_destroy(cr);

        /* Upload + first draw (static content, no repaint needed) */
        m_win.draw(1.0f);

        m_initialized = true;
        CVI_DEBUG("[wm-bg] running id={} {}x{}", WM_SID_BG, BG_WIDTH, BG_HEIGHT);
        return true;
    }

    cvi::expected<EXPECT_STATE, std::string> onRun() override
    {
        CVI_DEBUG("[wm-bg] onRun: entering wayland event loop");
        /* Static content — paint_cb = nullptr means dispatch only */
        m_win.run_loop(nullptr);
        return E_RUN_FINISH;
    }

    cvi::expected<bool, std::string> onSignal(int signum) override
    {
        CVI_DEBUG("[wm-bg] onSignal({}): requesting exit", signum);
        if (m_initialized)
            m_win.request_exit();
        return true;
    }

    cvi::expected<bool, std::string> onDeinitialize() override
    {
        CVI_DEBUG("[wm-bg] onDeinitialize: destroying window");
        m_win.destroy();
        m_initialized = false;
        return true;
    }

private:
    WmWindow  m_win;
    bool      m_initialized;
    std::string m_bg_image;
};

/* ---- framework entry point ---- */

std::shared_ptr<StandardProcessFramework> pStandardProcessFramework = nullptr;

std::shared_ptr<StandardProcessFramework>
createStandardProcessFramework(int32_t argc, const char *argv[])
{
    if (!pStandardProcessFramework) {
        pStandardProcessFramework = std::make_shared<BgApp>(
            "wm-bg", argc, argv);
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
