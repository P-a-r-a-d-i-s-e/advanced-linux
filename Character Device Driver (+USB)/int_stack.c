#include <linux/kernel.h>

#include <linux/init.h>

#include <linux/module.h>

#include <linux/kdev_t.h>

#include <linux/fs.h>

#include <linux/cdev.h>

#include <linux/device.h>

#include <linux/slab.h>

#include <linux/uaccess.h>

#include <linux/ioctl.h>

#include <linux/err.h>

#include <linux/errno.h>

#include <linux/usb.h>

#define USB_VENDOR_ID  (0x8564) //USB device's vendor ID
#define USB_PRODUCT_ID (0x1000) //USB device's product ID

#define DEVICE_NAME "int_stack"
#define SET_SIZE _IOW('a', 'a', int * )
#define DEFAULT_SIZE 3
DEFINE_SPINLOCK(stack_spinlock);

dev_t dev = 0;
static struct class *dev_class;
static struct cdev stack_cdev;

static int stack_size = DEFAULT_SIZE;
static int stack_real_size = 0;

typedef int T;

typedef struct Node_t {
    T value;
    struct Node_t *prev;
}
Node;

static Node *head;

/*
 ** Function Prototypes
 */
static int __init stack_init(void);
static void __exit stack_exit(void);
static int stack_open(struct inode *inode, struct file *file);
static int stack_release(struct inode *inode, struct file *file);
static ssize_t stack_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t stack_write(struct file *filp, const char *buf, size_t len, loff_t *off);
static long stack_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static void stack_usb_disconnect(struct usb_interface *interface);
static int stack_usb_probe(struct usb_interface *interface, const struct usb_device_id *id);

/*
 ** File operation sturcture
 */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = stack_read,
    .write = stack_write,
    .open = stack_open,
    .unlocked_ioctl = stack_ioctl,
    .release = stack_release,
};

static int stack_usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    /*Creating device*/
    if (IS_ERR(device_create(dev_class, NULL, dev, NULL, DEVICE_NAME))) {
        pr_err("Cannot create the Device 1\n");
        return -1;
    }
    return 0;

}

static void stack_usb_disconnect(struct usb_interface *interface) {
    pr_info("USB Device removed\n");
    device_destroy(dev_class, dev);
}

//usb_device_id provides a list of different types of USB devices that the driver supports
const struct usb_device_id stack_usb_table[] = {
    {
        USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID) //Put your USB device's Vendor and Product ID
    }, 
    {} /* Terminating entry */
};

//This enable the linux hotplug system to load the driver automatically when the device is plugged in
MODULE_DEVICE_TABLE(usb, stack_usb_table);

//The structure needs to do is register with the linux subsystem
static struct usb_driver stack_usb_driver = {
    .name = "Stack USB Driver",
    .probe = stack_usb_probe,
    .disconnect = stack_usb_disconnect,
    .id_table = stack_usb_table,
};

/*
 ** This function will be called when we open the Device file
 */
static int stack_open(struct inode *inode, struct file *file) {
    pr_info("Device File Opened...!!!\n");
    return 0;
}

/*
 ** This function will be called when we close the Device file
 */
static int stack_release(struct inode *inode, struct file *file) {
    pr_info("Device File Closed...!!!\n");
    return 0;
}

/*
 ** This function will be called when we read the Device file
 */
static ssize_t stack_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    T value;
    Node *out;

    if (head == NULL) {
        return -1;
    }

    spin_lock(&stack_spinlock);
    out = head;
    head = head->prev;
    value = out->value;

    if (copy_to_user(buf, &value, sizeof(T))) {
        return -EFAULT;
    }

    kfree(out);
    stack_real_size--;
    spin_unlock(&stack_spinlock);

    return sizeof(T);
}

/*
 ** This function will be called when we write the Device file
 */
static ssize_t stack_write(struct file *filp,
    const char __user *buf, size_t len, loff_t *off) {
    T value;
    Node *tmp;

    if (stack_real_size >= stack_size) {
        return -ERANGE;
    }

    if (len != sizeof(T)) {
        return -EINVAL;
    }

    if (copy_from_user(&value, buf, len)) {
        return -EFAULT;
    }

    spin_lock(&stack_spinlock);
    if ((tmp = kmalloc(sizeof(Node), GFP_KERNEL)) == 0) {
        printk(KERN_INFO "Cannot allocate memory in kernel\n");
        return -ENOMEM;
    }

    tmp->value = value;
    tmp->prev = head;

    head = tmp;
    stack_real_size++;
    spin_unlock(&stack_spinlock);

    return len;
}

/*
 ** This function will be called when we write IOCTL on the Device file
 */
static long stack_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
    case SET_SIZE:
        int new_size;

        if (copy_from_user(&new_size, (int *) arg, sizeof(int))) {
            return -EFAULT;
        }
        if (new_size <= 0) {
            return -EINVAL;
        }

        spin_lock(&stack_spinlock);
        stack_size = new_size;
        if (new_size < stack_real_size) {
            int offset = stack_real_size - new_size;
            Node *old;
            while (offset > 0) {
                old = head;
                head = head->prev;
                kfree(old);
                offset--;
                stack_real_size--;
            }
        }
        spin_unlock(&stack_spinlock);
        break;
    default:
        break;
    }
    return 0;
}

/*
 ** Module Init function
 */
static int __init stack_init(void) {
    /* Register this driver with the USB subsystem */
    int result;
    result = usb_register(&stack_usb_driver);
    if (result < 0) {
        pr_err("usb_register failed for the %s driver. Error number %d\n", stack_usb_driver.name, result);
        return -1;
    }

    /*Allocating Major number*/
    if ((alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME)) < 0) {
        pr_err("Cannot allocate major number\n");
        return -1;
    }
    pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

    /*Creating cdev structure*/
    cdev_init(&stack_cdev, &fops);

    /*Adding character device to the system*/
    if ((cdev_add(&stack_cdev, dev, 1)) < 0) {
        pr_err("Cannot add the device to the system\n");
        goto r_class;
    }

    /*Creating struct class*/
    if (IS_ERR(dev_class = class_create(DEVICE_NAME))) {
        pr_err("Cannot create the struct class\n");
        goto r_class;
    }

    head = NULL;

    pr_info("Device Driver Insert...Done!!!\n");
    return 0;

    r_device:
        class_destroy(dev_class);
    r_class:
        unregister_chrdev_region(dev, 1);
    return -1;
}

/*
 ** Module exit function
 */
static void __exit stack_exit(void) {
    usb_deregister(&stack_usb_driver);

    if (head) {
        Node *old;
        while (head) {
            old = head;
            head = head->prev;
            kfree(old);
        }
    }
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&stack_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Device Driver Remove...Done!!!\n");
}

module_init(stack_init);
module_exit(stack_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("paaizhboldin@kpfu.ru");
MODULE_DESCRIPTION("Kernel module implementing a stack of integers");