#ifndef LEDBLK_H
#define LEDBLK_H
#include <linux/ioctl.h>
#define LEDBLK_NAME "ledblk"
#define LEDBLK_MAJOR 239
#define LEDBLK_MAX_MINORS 16
#define LEDBLK_IOCTL_MAGIC 'L'
#define LEDBLK_IOCTL_SET_PATTERN _IOW(LEDBLK_IOCTL_MAGIC, 0, struct ledblk_pattern)
#define LEDBLK_IOCTL_GET_PATTERN _IOR(LEDBLK_IOCTL_MAGIC, 1, struct ledblk_pattern)
#define LEDBLK_IOCTL_SET_BRIGHTNESS _IOW(LEDBLK_IOCTL_MAGIC, 2, int)
#define LEDBLK_IOCTL_GET_BRIGHTNESS _IOR(LEDBLK_IOCTL_MAGIC, 3, int)
#define LEDBLK_IOCTL_MAXNR 3
struct ledblk_pattern {
    char pattern_name[64];
    unsigned int duration_ms; // Duration in milliseconds
    unsigned int repeat_count; // Number of times to repeat the pattern
};
#endif // LEDBLK_H