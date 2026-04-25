#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int read_value(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int val;
    fscanf(f, "%d", &val);
    fclose(f);
    return val;
}

int main(void)
{

   for(int i=0;i<10;i++) {
        int x = read_value("/sys/bus/iio/devices/iio:device0/in_accel_x_raw");
        int y = read_value("/sys/bus/iio/devices/iio:device0/in_accel_y_raw");
        int z = read_value("/sys/bus/iio/devices/iio:device0/in_accel_z_raw");

        printf("X=%d Y=%d Z=%d\n", x, y, z);
        sleep(1);
    }
}
