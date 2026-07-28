/*
 * wm-trans: send a command to the wm-transition weston module's socket.
 *   wm-trans transition 1200 9 300        # surface 1200, VIEW_FADE, 300ms
 *   wm-trans set_rect 1200 0 56 1024 584
 *   wm-trans set_opacity 1200 255
 *   wm-trans fade_layer 2000 in 300
 * Socket: $WM_TRANS_SOCK or $XDG_RUNTIME_DIR/wm-trans.sock.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <transition id type dur|set_rect id x y w h|set_opacity id 0..255|fade_layer lid in|out [dur]>\n", argv[0]);
        return 2;
    }

    const char *sock = getenv("WM_TRANS_SOCK");
    char def[256];
    if (!sock) {
        const char *xdg = getenv("XDG_RUNTIME_DIR");
        snprintf(def, sizeof(def), "%s/wm-trans.sock", xdg ? xdg : "/tmp");
        sock = def;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "connect %s: %m\n", sock);
        return 1;
    }

    char line[256];
    int off = 0;
    for (int i = 1; i < argc && off < (int)sizeof(line) - 2; ++i) {
        int k = snprintf(line + off, sizeof(line) - off - 1,
                         "%s%s", i > 1 ? " " : "", argv[i]);
        if (k < 0) break;
        off += k;
    }
    line[off++] = '\n';
    if (write(fd, line, (size_t)off) < 0) { perror("write"); close(fd); return 1; }

    char buf[1024];
    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n <= 0) break;
        fwrite(buf, 1, (size_t)n, stdout);
    }
    fflush(stdout);
    close(fd);
    return 0;
}
