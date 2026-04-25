#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>

#define RADIUS 20

// ?? Dessine une cible (carré)
void draw_target(unsigned char *fbp,
                 struct fb_fix_screeninfo finfo,
                 struct fb_var_screeninfo vinfo,
                 int x, int y) {

    for (int dy = -RADIUS; dy <= RADIUS; dy++) {
        for (int dx = -RADIUS; dx <= RADIUS; dx++) {

            int px = x + dx;
            int py = y + dy;

            if (px >= 0 && px < vinfo.xres &&
                py >= 0 && py < vinfo.yres) {

                long location = py * finfo.line_length +
                                px * (vinfo.bits_per_pixel / 8);

                *((unsigned int*)(fbp + location)) = 0x000000FF; // rouge
            }
        }
    }
}

// ?? Clean écran
void clear_screen(unsigned char *fbp, long size) {
    memset(fbp, 0x00, size);
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

    printf("Resolution: %dx%d\n", vinfo.xres, vinfo.yres);
    printf("Bits per pixel: %d\n", vinfo.bits_per_pixel);

    long screensize = finfo.smem_len;

    unsigned char *fbp = mmap(0, screensize,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED, fb, 0);

    if (fbp == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    struct input_event ev;

    int x = 0, y = 0;
    int touching = 0;

    // ?? Positions des cibles (coins)
    int targets[4][2] = {
        {40, 40},
        {vinfo.xres - 40, 40},
        {vinfo.xres - 40, vinfo.yres - 40},
        {40, vinfo.yres - 40}
    };

    int raw[4][2];

    clear_screen(fbp, screensize);

    printf("=== Calibration tactile ===\n");
    printf("Appuie sur les 4 cibles\n");

    for (int i = 0; i < 4; i++) {

        clear_screen(fbp, screensize);
        draw_target(fbp, finfo, vinfo, targets[i][0], targets[i][1]);

        printf("Point %d...\n", i + 1);

        while (1) {

            read(ts, &ev, sizeof(ev));

            if (ev.type == EV_ABS) {
                if (ev.code == ABS_MT_POSITION_X)
                    x = ev.value;
                if (ev.code == ABS_MT_POSITION_Y)
                    y = ev.value;
            }

            if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
                touching = ev.value;

                if (touching == 1) {

                    raw[i][0] = x;
                    raw[i][1] = y;

                    printf("Captured: X=%d Y=%d\n", x, y);

                    // attendre relâchement
                    while (touching) {
                        read(ts, &ev, sizeof(ev));
                        if (ev.type == EV_KEY && ev.code == BTN_TOUCH)
                            touching = ev.value;
                    }

                    break;
                }
            }
        }
    }

    // ?? Calcul min/max
    int xmin = raw[0][0], xmax = raw[0][0];
    int ymin = raw[0][1], ymax = raw[0][1];

    for (int i = 1; i < 4; i++) {
        if (raw[i][0] < xmin) xmin = raw[i][0];
        if (raw[i][0] > xmax) xmax = raw[i][0];
        if (raw[i][1] < ymin) ymin = raw[i][1];
        if (raw[i][1] > ymax) ymax = raw[i][1];
    }

    printf("\n=== RESULTAT ===\n");
    printf("X min = %d\n", xmin);
    printf("X max = %d\n", xmax);
    printf("Y min = %d\n", ymin);
    printf("Y max = %d\n", ymax);

    printf("\n=== A UTILISER ===\n");
    printf("#define TS_X_MIN %d\n", xmin);
    printf("#define TS_X_MAX %d\n", xmax);
    printf("#define TS_Y_MIN %d\n", ymin);
    printf("#define TS_Y_MAX %d\n", ymax);

    munmap(fbp, screensize);
    close(fb);
    close(ts);

    return 0;
}