// Software-only Linux misc character device. Emits a changing numeric sample.
// This is a teaching/test driver, not a physical I2C/SPI hardware driver.
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "sensor0"

static ssize_t sensor_read(struct file *file, char __user *user_buf,
                          size_t count, loff_t *ppos)
{
    char sample[32];
    int len;
    unsigned long tick = jiffies % 100;
    int centi = 2800 + (int)tick; /* 28.00 .. 28.99 */

    len = scnprintf(sample, sizeof(sample), "%d.%02d\n", centi / 100, centi % 100);
    return simple_read_from_buffer(user_buf, count, ppos, sample, len);
}

static const struct file_operations sensor_fops = {
    .owner = THIS_MODULE,
    .read = sensor_read,
    .llseek = no_llseek,
};

static struct miscdevice sensor_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &sensor_fops,
    .mode = 0444,
};

static int __init sensor_init(void)
{
    int ret = misc_register(&sensor_misc);
    if (ret)
        return ret;
    pr_info("virtual_sensor: registered /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit sensor_exit(void)
{
    misc_deregister(&sensor_misc);
    pr_info("virtual_sensor: unloaded\n");
}

module_init(sensor_init);
module_exit(sensor_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sampreeti Mohapatra");
MODULE_DESCRIPTION("Software-only virtual sensor misc character driver");
