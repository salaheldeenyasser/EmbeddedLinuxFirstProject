// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/ioctl.h>

#define DRIVER_NAME     "ledblk"
#define DEVICE_NAME     "ledblk0"
#define PROC_NAME       "ledblk"
#define CLASS_NAME      "ledblk"

#define BUF_SIZE        (1024 * 1024) /* 1 MB */

/* ioctl definitions */
#define LEDBLK_IOC_MAGIC   'L'
#define LEDBLK_CLEAR_BUF   _IO(LEDBLK_IOC_MAGIC, 0)
#define LEDBLK_GET_SIZE    _IOR(LEDBLK_IOC_MAGIC, 1, size_t)

struct ledblk_dev {
	dev_t devno;
	struct cdev cdev;
	struct class *class;
	struct device *device;

	char *buffer;
	size_t size;

	struct mutex lock;
	struct proc_dir_entry *proc;
};

static struct ledblk_dev ledblk;

/* ---------- file operations ---------- */

static int ledblk_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &ledblk;
	return 0;
}

static int ledblk_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t ledblk_read(struct file *filp, char __user *buf,
                           size_t count, loff_t *ppos)
{
        struct ledblk_dev *dev = filp->private_data;
        size_t available;
        size_t to_copy;
  
        if (*ppos >= dev->size)
            return 0;
  
        available = dev->size - (size_t)*ppos;
        to_copy = min(count, available);
  
        if (copy_to_user(buf, dev->buffer + *ppos, to_copy))
            return -EFAULT;
    
        *ppos += to_copy;
        return to_copy;
}


static ssize_t ledblk_write(struct file *filp, const char __user *buf,
                            size_t count, loff_t *ppos)
{
        struct ledblk_dev *dev = filp->private_data;
        size_t available;
        size_t to_copy;
  
        if (*ppos >= dev->size)
            return -ENOSPC;
    
        available = dev->size - (size_t)*ppos;
        to_copy = min(count, available);
    
        if (copy_from_user(dev->buffer + *ppos, buf, to_copy))
            return -EFAULT;
    
        *ppos += to_copy;
        return to_copy;
}


static loff_t ledblk_llseek(struct file *filp, loff_t off, int whence)
{
	struct ledblk_dev *dev = filp->private_data;
	loff_t newpos;

	switch (whence) {
	case SEEK_SET: newpos = off; break;
	case SEEK_CUR: newpos = filp->f_pos + off; break;
	case SEEK_END: newpos = dev->size + off; break;
	default: return -EINVAL;
	}

	if (newpos < 0 || newpos > dev->size)
		return -EINVAL;

	filp->f_pos = newpos;
	return newpos;
}

static long ledblk_ioctl(struct file *filp,
			 unsigned int cmd, unsigned long arg)
{
	struct ledblk_dev *dev = filp->private_data;

	switch (cmd) {
	case LEDBLK_CLEAR_BUF:
		memset(dev->buffer, 0, dev->size);
		break;

	case LEDBLK_GET_SIZE:
		if (copy_to_user((size_t __user *)arg,
				 &dev->size, sizeof(size_t)))
			return -EFAULT;
		break;

	default:
		return -ENOTTY;
	}

	return 0;
}

static int ledblk_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct ledblk_dev *dev = filp->private_data;
	unsigned long size = vma->vm_end - vma->vm_start;

	if (size > dev->size)
		return -EINVAL;

	return remap_vmalloc_range(vma, dev->buffer, 0);
}

static const struct file_operations ledblk_fops = {
	.owner          = THIS_MODULE,
	.open           = ledblk_open,
	.release        = ledblk_release,
	.read           = ledblk_read,
	.write          = ledblk_write,
	.llseek         = ledblk_llseek,
	.unlocked_ioctl = ledblk_ioctl,
	.mmap           = ledblk_mmap,
};

/* ---------- proc ---------- */

static ssize_t ledblk_proc_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	char tmp[64];
	int len;

	len = snprintf(tmp, sizeof(tmp),
		       "buffer_size=%d\n", BUF_SIZE);

	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static const struct proc_ops ledblk_proc_ops = {
	.proc_read = ledblk_proc_read,
};

/* ---------- sysfs ---------- */

static ssize_t size_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BUF_SIZE);
}

static DEVICE_ATTR_RO(size);

/* ---------- module init / exit ---------- */

static int __init ledblk_init(void)
{
	int ret;

	mutex_init(&ledblk.lock);

	ledblk.size = BUF_SIZE;
	ledblk.buffer = vmalloc(ledblk.size);
	if (!ledblk.buffer)
		return -ENOMEM;

	ret = alloc_chrdev_region(&ledblk.devno, 0, 1, DRIVER_NAME);
	if (ret)
		goto err_buf;

	cdev_init(&ledblk.cdev, &ledblk_fops);
	ret = cdev_add(&ledblk.cdev, ledblk.devno, 1);
	if (ret)
		goto err_chrdev;

	ledblk.class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(ledblk.class)) {
		ret = PTR_ERR(ledblk.class);
		goto err_cdev;
	}

	ledblk.device = device_create(ledblk.class, NULL,
				      ledblk.devno, NULL, DEVICE_NAME);
	if (IS_ERR(ledblk.device)) {
		ret = PTR_ERR(ledblk.device);
		goto err_class;
	}

	device_create_file(ledblk.device, &dev_attr_size);

	ledblk.proc = proc_create(PROC_NAME, 0444, NULL,
				  &ledblk_proc_ops);

	pr_info("ledblk loaded\n");
	return 0;

err_class:
	class_destroy(ledblk.class);
err_cdev:
	cdev_del(&ledblk.cdev);
err_chrdev:
	unregister_chrdev_region(ledblk.devno, 1);
err_buf:
	vfree(ledblk.buffer);
	return ret;
}

static void __exit ledblk_exit(void)
{
	proc_remove(ledblk.proc);
	device_remove_file(ledblk.device, &dev_attr_size);
	device_destroy(ledblk.class, ledblk.devno);
	class_destroy(ledblk.class);
	cdev_del(&ledblk.cdev);
	unregister_chrdev_region(ledblk.devno, 1);
	vfree(ledblk.buffer);
	pr_info("ledblk unloaded\n");
}

module_init(ledblk_init);
module_exit(ledblk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Salah Eldeen Yasser");
MODULE_DESCRIPTION("LED block character driver");
