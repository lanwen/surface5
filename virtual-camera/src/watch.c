#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

static int stream(const char *action) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("systemctl", "systemctl", "--user", action,
               "surface-virtual-camera.service", (char *)NULL);
        _exit(127);
    }
    if (pid < 0) return -1;
    int status;
    while (waitpid(pid, &status, 0) < 0) if (errno != EINTR) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
int main(void) {
    int fd = open("/dev/video42", O_RDWR | O_NONBLOCK);
    if (fd < 0) { perror("open video42"); return 1; }
    struct v4l2_event_subscription sub = {
        .type = V4L2_EVENT_PRIVATE_START + 0x08E00000 + 1,
        .flags = V4L2_EVENT_SUB_FL_SEND_INITIAL,
    };
    if (ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &sub) < 0) {
        perror("subscribe client usage"); return 1;
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    puts("Waiting for camera viewers");
    struct pollfd p = { .fd = fd, .events = POLLPRI };
    int idle_pending = 0;
    for (;;) {
        int rc = poll(&p, 1, idle_pending ? 3000 : -1);
        if (rc < 0) { if (errno == EINTR) continue; perror("poll"); return 1; }
        if (rc == 0) {
            puts("No viewers: stopping physical camera");
            if (stream("stop")) return 1;
            idle_pending = 0;
            continue;
        }
        if (p.revents & (POLLNVAL | POLLHUP)) return 1;
        if (!(p.revents & POLLPRI)) { fprintf(stderr, "Unexpected poll event\n"); return 1; }
        struct v4l2_event ev;
        while (ioctl(fd, VIDIOC_DQEVENT, &ev) == 0) {
            uint32_t viewers;
            __builtin_memcpy(&viewers, ev.u.data, sizeof(viewers));
            if (viewers) {
                idle_pending = 0;
                puts("Viewer connected: starting physical camera");
                if (stream("start")) return 1;
            } else idle_pending = 1;
        }
        if (errno != EAGAIN && errno != ENOENT) { perror("dequeue event"); return 1; }
    }
}
