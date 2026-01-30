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

#define DRIVER_NAME     "switchblk"
#define DEVICE_NAME     "switchblk0"
#define PROC_NAME       "switchblk"
#define CLASS_NAME      "switchblk"

#define BUF_SIZE        (1024 * 1024)

/* ioctl */
#define SWITCHBLK_IOC_MAGIC  'S'
#define SWITCHBLK_CLEAR_BUF  _IO(SWITCHBLK_IOC_MAGIC, 0)
#define SWITCHBLK_GET_SIZE   _IOR(SWITCHBLK_IOC_MAGIC, 1, size_t)

struct switchblk_dev {
	dev_t devno;
	struct cdev cdev;
	struct class *class;
	struct device *device;

	char *buffer;
	size_t size;

	struct mutex lock;
	struct proc_dir_entry *proc;
};

static struct switchblk_dev switchblk;

/* file operations — SAME as ledblk (intentionally) */

static int switchblk_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &switchblk;
	return 0;
}

static int switchblk_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t switchblk_read(struct file *filp, char __user *buf,
			      size_t count, loff_t *ppos)
{
	struct switchblk_dev *dev = filp->private_data;

	if (*ppos >= dev->size)
		return 0;

	count = min(count, dev->size - *ppos);

	mutex_lock(&dev->lock);
	if (copy_to_user(buf, dev->buffer + *ppos, count)) {
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	mutex_unlock(&dev->lock);

	*ppos += count;
	return count;
}

static ssize_t switchblk_write(struct file *filp, const char __user *buf,
			       size_t count, loff_t *ppos)
{
	struct switchblk_dev *dev = filp->private_data;

	if (*ppos >= dev->size)
		return -ENOSPC;

	count = min(count, dev->size - *ppos);

	mutex_lock(&dev->lock);
	if (copy_from_user(dev->buffer + *ppos, buf, count)) {
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	mutex_unlock(&dev->lock);

	*ppos += count;
	return count;
}

static loff_t switchblk_llseek(struct file *filp, loff_t off, int whence)
{
	struct switchblk_dev *dev = filp->private_data;
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

static long switchblk_ioctl(struct file *filp,
			    unsigned int cmd, unsigned long arg)
{
	struct switchblk_dev *dev = filp->private_data;

	switch (cmd) {
	case SWITCHBLK_CLEAR_BUF:
		memset(dev->buffer, 0, dev->size);
		break;

	case SWITCHBLK_GET_SIZE:
		if (copy_to_user((size_t __user *)arg,
				 &dev->size, sizeof(size_t)))
			return -EFAULT;
		break;

	default:
		return -ENOTTY;
	}

	return 0;
}

static int switchblk_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct switchblk_dev *dev = filp->private_data;
	unsigned long size = vma->vm_end - vma->vm_start;

	if (size > dev->size)
		return -EINVAL;

	return remap_vmalloc_range(vma, dev->buffer, 0);
}

static const struct file_operations switchblk_fops = {
	.owner          = THIS_MODULE,
	.open           = switchblk_open,
	.release        = switchblk_release,
	.read           = switchblk_read,
	.write          = switchblk_write,
	.llseek         = switchblk_llseek,
	.unlocked_ioctl = switchblk_ioctl,
	.mmap           = switchblk_mmap,
};

/* proc */

static ssize_t switchblk_proc_read(struct file *file, char __user *buf,
				   size_t count, loff_t *ppos)
{
	char tmp[64];
	int len;

	len = snprintf(tmp, sizeof(tmp),
		       "buffer_size=%d\n", BUF_SIZE);

	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static const struct proc_ops switchblk_proc_ops = {
	.proc_read = switchblk_proc_read,
};

/* sysfs */

static ssize_t size_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BUF_SIZE);
}

static DEVICE_ATTR_RO(size);

/* init / exit */

static int __init switchblk_init(void)
{
	int ret;

	mutex_init(&switchblk.lock);

	switchblk.size = BUF_SIZE;
	switchblk.buffer = vmalloc(switchblk.size);
	if (!switchblk.buffer)
		return -ENOMEM;

	ret = alloc_chrdev_region(&switchblk.devno, 0, 1, DRIVER_NAME);
	if (ret)
		goto err_buf;

	cdev_init(&switchblk.cdev, &switchblk_fops);
	ret = cdev_add(&switchblk.cdev, switchblk.devno, 1);
	if (ret)
		goto err_chrdev;

	switchblk.class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(switchblk.class)) {
		ret = PTR_ERR(switchblk.class);
		goto err_cdev;
	}

	switchblk.device = device_create(switchblk.class, NULL,
					 switchblk.devno, NULL, DEVICE_NAME);
	if (IS_ERR(switchblk.device)) {
		ret = PTR_ERR(switchblk.device);
		goto err_class;
	}

	device_create_file(switchblk.device, &dev_attr_size);
	switchblk.proc = proc_create(PROC_NAME, 0444, NULL,
				     &switchblk_proc_ops);

	pr_info("switchblk loaded\n");
	return 0;

err_class:
	class_destroy(switchblk.class);
err_cdev:
	cdev_del(&switchblk.cdev);
err_chrdev:
	unregister_chrdev_region(switchblk.devno, 1);
err_buf:
	vfree(switchblk.buffer);
	return ret;
}

static void __exit switchblk_exit(void)
{
	proc_remove(switchblk.proc);
	device_remove_file(switchblk.device, &dev_attr_size);
	device_destroy(switchblk.class, switchblk.devno);
	class_destroy(switchblk.class);
	cdev_del(&switchblk.cdev);
	unregister_chrdev_region(switchblk.devno, 1);
	vfree(switchblk.buffer);
	pr_info("switchblk unloaded\n");
}

module_init(switchblk_init);
module_exit(switchblk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Switch block character driver");
