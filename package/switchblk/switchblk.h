#ifndef SWITCHBLK_H
#define SWITCHBLK_H
#include <linux/ioctl.h>
#define SWITCHBLK_NAME "switchblk"
#define SWITCHBLK_MAJOR 240
#define SWITCHBLK_MAX_MINORS 16
#define SWITCHBLK_IOCTL_MAGIC 'S'

#define SWITCHBLK_IOCTL_ADD_DEVICE _IOW(SWITCHBLK_IOCTL_MAGIC, 0, struct switchblk_device_info)
#define SWITCHBLK_IOCTL_REMOVE_DEVICE _IOW(SWITCHBLK_IOCTL_MAGIC, 1, int)
#define SWITCHBLK_IOCTL_LIST_DEVICES _IOR(SWITCHBLK_IOCTL_MAGIC, 2, struct switchblk_device_list)  
#define SWITCHBLK_IOCTL_GET_DEVICE_INFO _IOR(SWITCHBLK_IOCTL_MAGIC, 3, struct switchblk_device_info)
#define SWITCHBLK_IOCTL_SET_DEVICE_STATE _IOW(SWITCHBLK_IOCTL_MAGIC, 4, struct switchblk_device_state)
#define SWITCHBLK_IOCTL_GET_DEVICE_STATE _IOR(SWITCHBLK_IOCTL_MAGIC, 5, struct switchblk_device_state)
#define SWITCHBLK_IOCTL_MAXNR 5
struct switchblk_device_info {
    int device_id;
    char device_name[64];
    unsigned long capacity; // in bytes
    char status[32];
};
struct switchblk_device_state {
    int device_id;
    bool active; // true for active, false for inactive
};
struct switchblk_device_list {
    int count;
    struct switchblk_device_info devices[SWITCHBLK_MAX_MINORS];
};

 

#endif // SWITCHBLK_H
