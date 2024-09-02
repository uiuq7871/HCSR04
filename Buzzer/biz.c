#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/delay.h>

#define DEVICE_NAME "buzzer"
#define BUZZER_PIN 27

static int major_number;
static struct class *buzzer_class = NULL;
static struct device *buzzer_device = NULL;

void gpio_out(int pin, int value) {
    gpio_direction_output(pin, value);
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
    char input[10];
    if (len > sizeof(input) - 1) {
        return -EINVAL;
    }

    if (copy_from_user(input, buffer, len)) {
        return -EFAULT;
    }

    input[len] = '\0';

    if (strcmp(input, "ON") == 0) {
        gpio_out(BUZZER_PIN, 1);
        msleep(100);
        gpio_out(BUZZER_PIN, 0);
    } else if (strcmp(input, "OFF") == 0) {
        gpio_out(BUZZER_PIN, 0);
    } else {
        return -EINVAL;
    }

    return len;
}

static int device_open(struct inode *inode, struct file *file) {
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    return 0;
}

static struct file_operations fops = {
    .open = device_open,
    .release = device_release,
    .write = device_write,
};

static int __init buzzer_init(void) {
    if (!gpio_is_valid(BUZZER_PIN)) {
        printk(KERN_ALERT "Invalid GPIO pin\n");
        return -ENODEV;
    }

    if (gpio_request(BUZZER_PIN, DEVICE_NAME)) {
        printk(KERN_ALERT "Failed to request GPIO pin\n");
        return -EBUSY;
    }

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "Failed to register character device\n");
        gpio_free(BUZZER_PIN);
        return major_number;
    }

    buzzer_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(buzzer_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        gpio_free(BUZZER_PIN);
        printk(KERN_ALERT "Failed to create device class\n");
        return PTR_ERR(buzzer_class);
    }

    buzzer_device = device_create(buzzer_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(buzzer_device)) {
        class_destroy(buzzer_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        gpio_free(BUZZER_PIN);
        printk(KERN_ALERT "Failed to create device\n");
        return PTR_ERR(buzzer_device);
    }

    printk(KERN_INFO "Buzzer device initialized\n");
    return 0;
}

static void __exit buzzer_exit(void) {
    device_destroy(buzzer_class, MKDEV(major_number, 0));
    class_unregister(buzzer_class);
    class_destroy(buzzer_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    gpio_free(BUZZER_PIN);
    printk(KERN_INFO "Buzzer module unloaded\n");
}

module_init(buzzer_init);
module_exit(buzzer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ting");
MODULE_DESCRIPTION("Buzzer driver");
