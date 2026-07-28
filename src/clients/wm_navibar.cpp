/*
 * wm-navibar: left sidebar navigation bar.
 *
 * Standalone process. Creates an ivi_surface on the left side of the screen,
 * shows icon buttons for home/radio/music/navi, and sends IPC commands via
 * IpcFactory on touch/click.
 *
 * 按键 press 效果：压住时图标上覆盖半透明黑层，松开后恢复。
 */

#include "wm/ipc_channel.h"
#include "wm/WmWindow.h"
#include "wm/surface_ids.h"

#include <cairo/cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <linux/input-event-codes.h>
#include <wayland-client-core.h>

/* ---- 布局 ---- */
#define NAVI_ICON_SZ  80
#define NAVI_PAD      16
#define NAVI_WIDTH    WM_NAVIBAR_W   /* 96 */

/* ---- 运行时资源根路径 ---- */
static std::string g_res_root = "res";

/* ---- 按钮表 ---- */
struct NaviButton {
    const char *name;
    const char *file;     /* 相对于 res_root */
};

static const NaviButton k_buttons[] = {
    { "home",  "navibar/20001.png" },
    { "radio", "navibar/20002.png" },
    { "music", "navibar/20003.png" },
    { "navi",  "navibar/20004.png" },
    { "",      "navibar/20005.png" },   /* 预留 */
};
static const int k_btn_cnt = sizeof(k_buttons) / sizeof(k_buttons[0]);

static int btn_y(int idx)
{
    return NAVI_PAD + idx * (NAVI_ICON_SZ + NAVI_PAD);
}
static int total_height(void)
{
    return NAVI_PAD + k_btn_cnt * (NAVI_ICON_SZ + NAVI_PAD);
}

/* =================================================================
 * NavibarApp
 * ================================================================*/

class NavibarApp {
public:
    NavibarApp() = default;
    ~NavibarApp() { deinit(); }

    bool init()
    {
        // --- IPC 客户端 ---
        m_ipc = IpcFactory::create(IpcType::SOCKET, "wm-trans");
        if (!m_ipc->init(IpcRole::CLIENT)) {
            fprintf(stderr, "[navibar] IPC init failed\n");
            return false;
        }
        if (!m_ipc->connect()) {
            fprintf(stderr, "[navibar] IPC connect failed, will retry\n");
        }

        // --- 窗口 ---
        if (!m_win.create(WM_SID_NAVIBAR, NAVI_WIDTH, total_height())) {
            fprintf(stderr, "[navibar] WmWindow::create failed\n");
            return false;
        }

        // 注册输入回调
        m_win.set_pointer_motion_cb([this](wl_fixed_t sx, wl_fixed_t sy) {
            m_last_x = wl_fixed_to_double(sx);
            m_last_y = wl_fixed_to_double(sy);
        });
        m_win.set_pointer_button_cb([this](uint32_t button, uint32_t state) {
            if (button == BTN_LEFT) {
                if (state == 1) onPress(m_last_x, m_last_y);
                else            onRelease(m_last_x, m_last_y);
            }
        });
        m_win.set_touch_down_cb([this](int32_t id, wl_fixed_t x, wl_fixed_t y) {
            (void)id;
            m_last_x = wl_fixed_to_double(x);
            m_last_y = wl_fixed_to_double(y);
            onPress(m_last_x, m_last_y);
        });
        m_win.set_touch_up_cb([this](int32_t) {
            if (m_pressed_idx >= 0)
                onRelease(m_last_x, m_last_y);
        });

        m_inited = true;
        fprintf(stdout, "[navibar] ready, %d buttons, res=%s\n",
                k_btn_cnt, g_res_root.c_str());
        return true;
    }

    void run()
    {
        if (!m_inited) return;
        m_win.run_loop([this](cairo_t *cr, int w, int h) {
            // 轮询 IPC 回复（非阻塞）
            if (m_ipc && m_ipc->isConnected()) {
                struct pollfd pfd = {m_ipc->getFd(), POLLIN, 0};
                if (poll(&pfd, 1, 0) > 0)
                    m_ipc->onReadable();
            }
            paint(cr, w, h);
        });
    }

    void deinit()
    {
        m_win.destroy();
        m_inited = false;
    }

private:
    void paint(cairo_t *cr, int width, int height)
    {
        // 背景
        cairo_set_source_rgb(cr,
            ((WM_NAVIBAR_BG >> 16) & 0xff) / 255.0,
            ((WM_NAVIBAR_BG >> 8)  & 0xff) / 255.0,
            ((WM_NAVIBAR_BG)       & 0xff) / 255.0);
        cairo_paint(cr);

        int cx = (NAVI_WIDTH - NAVI_ICON_SZ) / 2;

        for (int i = 0; i < k_btn_cnt; ++i) {
            int y = btn_y(i);

            // 动态加载图标（懒加载，首次 paint 时加载）
            if (!m_icons[i].surf) {
                m_icons[i] = loadIcon(k_buttons[i].file);
            }

            // 画图标
            if (m_icons[i].surf) {
                cairo_set_source_surface(cr, m_icons[i].surf, cx, y);
                cairo_paint(cr);
            } else {
                // 占位色块
                cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
                cairo_rectangle(cr, cx, y, NAVI_ICON_SZ, NAVI_ICON_SZ);
                cairo_fill(cr);
            }

            // press 效果
            if (m_pressed_idx == i) {
                cairo_set_source_rgba(cr, 0, 0, 0, 0.35);
                cairo_rectangle(cr, cx, y, NAVI_ICON_SZ, NAVI_ICON_SZ);
                cairo_fill(cr);
            }
        }
    }

    struct CachedIcon {
        cairo_surface_t *surf = nullptr;
        int w = 0, h = 0;
    };

    CachedIcon loadIcon(const std::string& relpath)
    {
        std::string full = g_res_root + "/" + relpath;
        cairo_surface_t *s = cairo_image_surface_create_from_png(full.c_str());
        if (cairo_surface_status(s) != CAIRO_STATUS_SUCCESS) {
            fprintf(stderr, "[navibar] load %s failed\n", full.c_str());
            return {};
        }
        return {s,
                cairo_image_surface_get_width(s),
                cairo_image_surface_get_height(s)};
    }

    int hitTest(double x, double y) const
    {
        int cx = (NAVI_WIDTH - NAVI_ICON_SZ) / 2;
        for (int i = 0; i < k_btn_cnt; ++i) {
            int by = btn_y(i);
            if (x >= cx && x < cx + NAVI_ICON_SZ &&
                y >= by && y < by + NAVI_ICON_SZ) {
                return i;
            }
        }
        return -1;
    }

    void onPress(double x, double y)
    {
        int idx = hitTest(x, y);
        if (idx >= 0 && idx != m_pressed_idx) {
            m_pressed_idx = idx;
            m_need_redraw = true;
        }
    }

    void onRelease(double x, double y)
    {
        int idx = m_pressed_idx;
        m_pressed_idx = -1;
        m_need_redraw = true;

        // 没按在按钮上，或按到预留按钮，不发送 IPC
        if (idx < 0 || idx >= k_btn_cnt)
            return;

        const char *cmd_name = k_buttons[idx].name;
        if (!cmd_name || cmd_name[0] == '\0')
            return;

        if (!m_ipc->isConnected())
            m_ipc->connect();

        if (m_ipc->isConnected()) {
            std::string line = std::string("activate ") + cmd_name;
            m_ipc->sendLine(line, [](const std::string &reply) {
                fprintf(stdout, "[navibar] reply: %s\n", reply.c_str());
            });
            fprintf(stdout, "[navibar] send: %s\n", line.c_str());
        } else {
            fprintf(stderr, "[navibar] IPC not connected\n");
        }
    }

    // --- 成员 ---
    WmWindow                    m_win;
    std::shared_ptr<IIpcChannel> m_ipc;
    CachedIcon                  m_icons[5] = {};
    double                      m_last_x = 0;
    double                      m_last_y = 0;
    int                         m_pressed_idx = -1;
    bool                        m_need_redraw = false;
    bool                        m_inited = false;
};

/* =================================================================
 * main
 * ================================================================*/

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    (void)argc;

    // 自动探测 res 根路径
    if (argv[0] && strchr(argv[0], '/')) {
        std::string self = argv[0];
        size_t slash = self.rfind('/');
        if (slash != std::string::npos) {
            std::string dir = self.substr(0, slash);
            std::string candidate = dir + "/../res";
            if (access(candidate.c_str(), F_OK) == 0)
                g_res_root = candidate;
        }
    }

    NavibarApp app;
    if (!app.init()) {
        fprintf(stderr, "[navibar] init failed\n");
        return 1;
    }
    app.run();
    app.deinit();
    return 0;
}
