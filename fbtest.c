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
    unsigned char* fbp = 0;
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

    if(vinfo.bits_per_pixel != 32)
    {
        printf("We don't support 16bit color!\n");
        exit(EXIT_FAILURE);
    }

//    fprintf(stderr, "%d %d %d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    screen_size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    fbp = (unsigned char*)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fbfd, 0);
    if ((int64_t)fbp == -1)
    {
        perror("Error: failed to map framebuffer device to memory");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    unsigned char red, green, blue;
    int xstart, ystart;
    int pixel_size = 10;
    bool red_enabled = false, green_enabled = false, blue_enabled = false;
    int sleep_usecs = 0;

    char c;
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
            sleep_usecs = atoi(optarg);
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
            printf("  -t                       Time of sleep() in usecs.\n");
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

    while(true)
    {
        if(ystart >= vinfo.yres)
        {
            xstart = 0;
            ystart = 0;
        }
        for (int y = ystart; y < ystart + pixel_size && y < vinfo.yres; y++)
        {
            if(y % pixel_size == 0)
            {
                if(red_enabled) red = rand();
                if(green_enabled) green = rand();
                if(blue_enabled) blue = rand();
            }
            for (int x = xstart; x < xstart + pixel_size && x < vinfo.xres; x++)
            {
                location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
                *(fbp + location) = blue;
                *(fbp + location + 1) = green;
                *(fbp + location + 2) = red;
                *(fbp + location + 3) = 0;
            }
            if(sleep_usecs > 0) usleep(sleep_usecs);
        }
        xstart += pixel_size;
        if(xstart >= vinfo.xres)
        {
            xstart = 0;
            ystart += pixel_size;
        }
    }

    munmap(fbp, screen_size);
    close(fbfd);
    close(ttyfd);

    return EXIT_SUCCESS;
}
