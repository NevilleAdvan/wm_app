/*
 * wm-ctl: send a command to the WM brain — the wm-transition module.
 *
 * Uses IIpcChannel + IpcFactory so the underlying transport can be switched
 * (socket ↔ fdbus ↔ gdbus) without changing this code.
 *
 *   wm-ctl focus 1200
 *   wm-ctl list
 *   wm-ctl activate home
 *   wm-ctl apps
 *   wm-ctl stack
 */

#include "wm/ipc_channel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args]\n", argv[0]);
        fprintf(stderr, "Commands:\n");
        fprintf(stderr, "  focus <id>           Bring surface to top\n");
        fprintf(stderr, "  list                 List surfaces\n");
        fprintf(stderr, "  activate <name>      Activate app (focus-or-launch)\n");
        fprintf(stderr, "  home                 Go to home screen\n");
        fprintf(stderr, "  back                 Pop focus stack\n");
        fprintf(stderr, "  apps                 List registered apps\n");
        fprintf(stderr, "  stack                Show focus stack\n");
        fprintf(stderr, "  transition <id> <type> <dur>\n");
        fprintf(stderr, "  set_rect <id> <x> <y> <w> <h>\n");
        fprintf(stderr, "  set_opacity <id> <0..255>\n");
        fprintf(stderr, "  set_visible <id> <0|1>\n");
        fprintf(stderr, "  commit\n");
        return 2;
    }

    // 创建 IPC 客户端（工厂）
    auto ipc = IpcFactory::create(IpcType::SOCKET, "wm-trans");
    if (!ipc->init(IpcRole::CLIENT)) {
        fprintf(stderr, "init failed\n");
        return 1;
    }
    if (!ipc->connect()) {
        fprintf(stderr, "connect to wm-trans.sock failed\n");
        return 1;
    }

    // 拼接命令
    std::string line;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) line += " ";
        line += argv[i];
    }

    // 发送 + 接收回复
    bool replied = false;
    if (!ipc->sendLine(line, [&](const std::string& reply) {
        printf("%s\n", reply.c_str());
        replied = true;
    })) {
        fprintf(stderr, "send failed\n");
        return 1;
    }

    // 等待回复（轮询 onReadable，最多等 3 秒）
    for (int i = 0; i < 300; ++i) {
        if (replied)
            break;

        struct timeval tv = {0, 10000};  // 10ms
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(ipc->getFd(), &rfds);
        if (select(ipc->getFd() + 1, &rfds, NULL, NULL, &tv) > 0) {
            ipc->onReadable();
            if (replied)
                break;
        }
    }

    if (!replied) {
        fprintf(stderr, "no reply received\n");
        return 1;
    }

    return 0;
}
