#include <stdio.h>
#include "print_usage.h"
void print_usage()
{
    printf("Usage: fbtest [OPTION]...\n");
    printf("Framebuffer test.\n");
    printf("Options:\n");
    printf("  -f                       Set fbdev path other than /dev/fb0\n");
    printf("  -s                       Size of an pixel.\n");
    printf("  -t                       Time of sleep() in nsecs.\n");
    printf("  -r                       Turn on red color.\n");
    printf("  -g                       Turn on green color.\n");
    printf("  -b                       Turn on blue color.\n");
    printf("  -h                       Print this help message.\n");
}
