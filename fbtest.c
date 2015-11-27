#include <fcntl.h>
#include <getopt.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

void exit_handler()
{
    int ttyfd = open("/dev/tty", O_RDWR);
    ioctl(ttyfd, KDSETMODE, KD_TEXT);
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    int ttyfd, fbfd;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screen_size = 0;
    uint8_t* fbp = 0;
    long int location = 0;

    ttyfd = open("/dev/tty", O_RDWR);
    if(ttyfd == -1)
    {
        perror("Error: cannot open tty device");
        exit(EXIT_FAILURE);
    }

    fbfd = open("/dev/fb0", O_RDWR);

    if(fbfd == -1)
    {
        perror("Error: cannot open framebuffer device");
        exit(EXIT_FAILURE);
    }

    if(ioctl(ttyfd, KDSETMODE, KD_GRAPHICS) == -1)
    {
        perror("Error: cannot set tty to graphics mode");
        exit(EXIT_FAILURE);
    }
    else
    {
        signal(SIGINT, exit_handler);
        signal(SIGTERM, exit_handler);
    }

    if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror("Error reading fixed information");
        exit(EXIT_FAILURE);
    }

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "%d %d %d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    screen_size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    fbp = (uint8_t*)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fbfd, 0);
    if ((int)fbp == -1)
    {
        perror("Error: failed to map framebuffer device to memory");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    uint32_t red, green, blue;
    int xstart, ystart;
    int pixel_size = 10;
    bool red_enabled = false, green_enabled = false, blue_enabled = false;
    struct timespec interval = {.tv_sec = 0, .tv_nsec = 10};
    signed char c;
    bool color_options_exist = false;
    while((c = getopt(argc, argv, "s:t:rgbh")) != -1)
    {
        switch(c)
        {
        case 's':
            pixel_size = atoi(optarg);
            if(pixel_size == 0)
            {
                perror("Error: Invalid pixel_size\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 't':
            interval.tv_nsec = atol(optarg);
            break;
        case 'r':
            color_options_exist = true;
            red_enabled = true;
            break;
        case 'g':
            color_options_exist = true;
            green_enabled = true;
            break;
        case 'b':
            color_options_exist = true;
            blue_enabled = true;
            break;
        case 'h':
            printf("Usage: fbtest [OPTION]...\n");
            printf("Framebuffer test.\n");
            printf("Options:\n");
            printf("  -s                       Size of an pixel.\n");
            printf("  -t                       Time of sleep() in nsecs.\n");
            printf("  -r                       Turn on red color.\n");
            printf("  -g                       Turn on green color.\n");
            printf("  -b                       Turn on blue color.\n");
            printf("  -h                       Print this help message.\n");

            exit(EXIT_SUCCESS);
        case '?':
            fprintf(stderr, "Unknown option -%c\n", optopt);
            break;
        }
    }

    if(!color_options_exist)
    {
        red_enabled = blue_enabled = green_enabled = true;
    }

    do
    {
        if(ystart >= vinfo.yres)
        {
            xstart = 0;
            ystart = 0;
        }
        int y;
        for (y = ystart; y < ystart + pixel_size && y < vinfo.yres; y++)
        {
            if(y % pixel_size == 0)
            {
                if(red_enabled) red = rand();
                if(green_enabled) green = rand();
                if(blue_enabled) blue = rand();
                switch(vinfo.bits_per_pixel)
                {
                case 8:
                    red = red % 3;
                    green = green % 3;
                    blue = blue % 2;
                    break;
                case 16:
                    red = red % 32;
                    green = green % 64;
                    blue = blue % 32;
                    break;
                case 32:
                    red = red % 256;
                    green = green % 256;
                    blue = blue % 256;
                    break;
                }
            }
            int x;
            for (x = xstart; x < xstart + pixel_size && x < vinfo.xres; x++)
            {
                location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
                switch(vinfo.bits_per_pixel)
                {
                case 8:
                    *(fbp + location) = (red << 5) | (green << 2) | blue;
                    break;
                case 16:
                    *(uint16_t*)(fbp + location) = (red << 11) | (green << 5) | blue;
                    break;
                case 32:
                    *(fbp + location) = (uint8_t)blue;
                    *(fbp + location + 1) = (uint8_t)green;
                    *(fbp + location + 2) = (uint8_t)red;
                    *(fbp + location + 3) = (uint8_t)0;
                    break;
                }
            }
            if(interval.tv_nsec > 0) nanosleep(&interval, NULL);
        }
        xstart += pixel_size;
        if(xstart >= vinfo.xres)
        {
            xstart = 0;
            ystart += pixel_size;
        }
    } while(true);

    munmap(fbp, screen_size);
    close(fbfd);
    close(ttyfd);

    return EXIT_SUCCESS;
}
