#include <linux/module.h>
#include <linux/tty.h>

#include <linux/kd.h>
#include <linux/vt.h>

#include <linux/vt_kern.h>
#include <linux/console_struct.h>
#include <linux/proc_fs.h>

#include <linux/uaccess.h>

#include <linux/timer.h>

MODULE_DESCRIPTION("LED blinker");
MODULE_AUTHOR("Grace T");
MODULE_LICENSE("GPL");

static int all_leds_on = 7;
static int blink_divisor = 5;
#define RESTORE_LEDS 0xFF

static struct timer_list my_timer;
static struct tty_driver *my_driver;
static char kbledstatus = 0;

static void my_timer_func(struct timer_list *timers){
	int *pstatus = (int *)&kbledstatus;
	if (*pstatus == all_leds_on)
		*pstatus = RESTORE_LEDS;
	else
		*pstatus = all_leds_on;

	(my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty,
			KDSETLED, *pstatus);

	my_timer.expires = jiffies + HZ / (blink_divisor ? blink_divisor : 1);
	add_timer(&my_timer);
}

static ssize_t kbleds_write(struct file *f, const char __user *buf, size_t count, loff_t * pos)
{
	char kbuf[3];
	int digit;
	if (count < 2)
		return -EINVAL;
	if (copy_from_user(kbuf, buf, 2))
		return -EFAULT;
	kbuf[2] = '\0';
	if (kbuf[1] < '0' || kbuf[1] > '9')
		return -EINVAL;
	digit = kbuf[1] - '0';
	if (kbuf[0] == 'L' && digit <= 7){
		all_leds_on = digit;
		printk(KERN_INFO "kbleds: LED mask set to %d\n", digit);
	}	
	else if (kbuf[0] == 'D') {
		blink_divisor = digit;
		printk(KERN_INFO "kbleds: divisor set to %d\n", digit);
	}
	else return -EINVAL;
	return count;
}

static const struct proc_ops kbleds_ops = {
	.proc_write = kbleds_write,
};

static struct proc_dir_entry *kbleds_entry;

static int __init kbleds_init(void)
{
	kbleds_entry = proc_create("kbleds", 0666, NULL, &kbleds_ops);
	my_driver = vc_cons[fg_console].d->port.tty->driver;
	timer_setup(&my_timer, my_timer_func, 0);
	my_timer.expires = jiffies + HZ / blink_divisor;
	add_timer(&my_timer);

	return 0;
}

static void __exit kbleds_cleanup(void)
{
	timer_shutdown_sync(&my_timer);
	proc_remove(kbleds_entry);
	(my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, RESTORE_LEDS);
}

module_init(kbleds_init);
module_exit(kbleds_cleanup);
