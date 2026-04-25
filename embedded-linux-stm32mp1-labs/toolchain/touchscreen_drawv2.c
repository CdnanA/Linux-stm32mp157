#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>

#define FB_WIDTH 480
#define FB_HEIGHT 800

#define TS_X_MIN 50
#define TS_X_MAX 455
#define TS_Y_MIN 40
#define TS_Y_MAX 771

static inline int clamp(int v, int min, int max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

int main() {

    int fb = open("/dev/fb0", O_RDWR);
    int ts = open("/dev/input/event0", O_RDONLY);

    if (fb < 0 || ts < 0) {
        perror("open");
        return 1;
    }

    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;

    ioctl(fb, FBIOGET_FSCREENINFO, &finfo);
    ioctl(fb, FBIOGET_VSCREENINFO, &vinfo);

    long screensize = finfo.smem_len;

    unsigned char *fbp = mmap(0, screensize,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED, fb, 0);

    if (fbp == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    memset(fbp, 0x00, screensize);

    struct input_event ev;

    int x = 0, y = 0;
    int touching = 0;

    printf("Touch draw OK FINAL\n");

    while (1) {

        read(ts, &ev, sizeof(ev));

        if (ev.type == EV_ABS) {
            if (ev.code == ABS_MT_POSITION_X)
                x = ev.value;
            if (ev.code == ABS_MT_POSITION_Y)
                y = ev.value;
        }

        if (ev.type == EV_KEY && ev.code == BTN_TOUCH)
            touching = ev.value;

        if (touching) {

            // X correct
            int draw_x = (x - TS_X_MIN) * FB_WIDTH / (TS_X_MAX - TS_X_MIN);

            // ?? Y corrigé (PAS inversé)
            int draw_y = (y - TS_Y_MIN) * FB_HEIGHT / (TS_Y_MAX - TS_Y_MIN);

            draw_x = clamp(draw_x, 0, FB_WIDTH - 1);
            draw_y = clamp(draw_y, 0, FB_HEIGHT - 1);

            long location = draw_y * finfo.line_length +
                            draw_x * (vinfo.bits_per_pixel / 8);

            *((unsigned int*)(fbp + location)) = 0x000000FF;
        }
    }

    munmap(fbp, screensize);
    close(fb);
    close(ts);

    return 0;
}