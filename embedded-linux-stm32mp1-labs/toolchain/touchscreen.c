#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

int main() {
    int fd = open("/dev/input/event0", O_RDONLY);
    struct input_event ev;

    if (fd < 0) {
        perror("open");
        return 1;
    }

    while (1) {
        read(fd, &ev, sizeof(ev));

        if (ev.type == EV_ABS) {
            if (ev.code == ABS_X)
                printf("X: %d\n", ev.value);
            if (ev.code == ABS_Y)
                printf("Y: %d\n", ev.value);
        }

        if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
            printf("Touch: %d\n", ev.value);
        }
    }
}
