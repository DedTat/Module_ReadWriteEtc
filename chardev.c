#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include "chardev.h"

#define SUCCESS 0
#define DEVICE_NAME "chardev"
#define BUF_LEN 80
#define DEBUG 0

static int  DirectionOutput = 1;
static int  Device_Open = 0;
static char Message[BUF_LEN];
static char MessageTemp[BUF_LEN];
static char MessageDelete[BUF_LEN] = "delete\n";
static char MessageDirectionForward[BUF_LEN] = "direction forward\n";
static char MessageDirectionBack[BUF_LEN] = "direction back\n";
static char *Message_Ptr;

static int device_open(struct inode *inode, struct file *file)
{
	#ifdef DEBUG
		printk(KERN_INFO "device_open(%p)\n", file);
	#endif

	if (Device_Open)
	{
		return -EBUSY;
	}

	Message_Ptr = Message;

	Device_Open++;
	try_module_get(THIS_MODULE);
	return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
	#ifdef DEBUG
		printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
	#endif

	Device_Open--;
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t device_read(struct file *file, char __user * buffer, size_t length, loff_t * offset)
{
	int 	bytes_read = 0;
	char 	MessageBuf[BUF_LEN];
	static char *MessageBuf_Ptr;
	int 	i = 0;
	char 	temp;

	#ifdef DEBUG
		printk(KERN_INFO "device_read(%p,%p,%d)\n", file, buffer, (int)length);
	#endif

	memset(MessageTemp, 0, sizeof(MessageTemp));

	if (*Message_Ptr == 0)
	{
		return 0;
	}

	if(DirectionOutput == 1)
	{
		while (length && *Message_Ptr)
		{
			MessageTemp[bytes_read++] = *(Message_Ptr);
			put_user(*(Message_Ptr++), buffer++);
			length--;
		}
	}
	else
	{
		#ifdef DEBUG
			printk(KERN_INFO "Print string rollback");
		#endif

		strcpy(MessageBuf, Message);
		for(i = 0; i < strlen(MessageBuf) / 2; i++)
		{
			temp = MessageBuf[i];
			MessageBuf[i] = MessageBuf[strlen(MessageBuf) - i - 1];
			MessageBuf[strlen(MessageBuf) - i - 1] = temp;
		}
		MessageBuf_Ptr = MessageBuf;

		printk(KERN_INFO "String to out: %s", MessageBuf_Ptr);

		while (length && *MessageBuf_Ptr)
		{
			MessageTemp[bytes_read++] = *(MessageBuf_Ptr);
			put_user(*(MessageBuf_Ptr++), buffer++);
			length--;
		}
	}

	#ifdef DEBUG
		printk(KERN_INFO "Read %d bytes, %d left\n", bytes_read, (int)length);
	#endif

	return bytes_read;
}

static ssize_t device_write(struct file *file, const char __user * buffer, size_t length, loff_t * offset)
{
	int i;

	#ifdef DEBUG
		printk(KERN_INFO "device_write(%p, %s, %d)", file, buffer, (int)length);
	#endif

	for (i = 0; i < length && i < BUF_LEN; i++)
	{
        	get_user(MessageTemp[i], buffer + i);
	}

	if (strcmp(MessageDelete, MessageTemp) == 0)
	{
		#ifdef DEBUG
			printk(KERN_INFO "Recieve Message delete command -> clear message");
		#endif
		memset(Message, 0, sizeof(Message));
	}
	else if (strcmp(MessageDirectionForward, MessageTemp) == 0)
	{
		#ifdef DEBUG
			printk(KERN_INFO "Recieve Message forward direction command -> set forward direction");
		#endif
		DirectionOutput = 1;
	}
	else if (strcmp(MessageDirectionBack, MessageTemp) == 0)
	{
		#ifdef DEBUG
			printk(KERN_INFO "Recieve Message back direction command -> set back direction");
		#endif
		DirectionOutput = 0;
	}
	else
	{
		memset(Message, 0, sizeof(Message));
		strcpy(Message, MessageTemp);
		Message[sizeof(Message) - 1] = 0;
	}

	Message_Ptr = Message;

	return i;
}

static long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	int i;
	char *temp;
	char ch;

	switch (ioctl_num)
	{
		case IOCTL_SET_MSG:
		{
			temp = (char *)ioctl_param;

			get_user(ch, temp);

			for (i = 0; ch && i < BUF_LEN; i++, temp++)
			{
				get_user(ch, temp);
      			}

    			device_write(file, (char *)ioctl_param, i, 0);
      			break;
		}

		case IOCTL_GET_MSG:
		{
			i = device_read(file, (char *)ioctl_param, 99, 0);

			put_user('\0', (char *)ioctl_param + i);
      			break;
		}

		case IOCTL_GET_NTH_BYTE:
		{
			return Message[ioctl_param];
			break;
		}

		case IOCTL_WRITE_FILE:
		{
			unsigned long long offset = 0;
			struct file* filp = NULL;
			mm_segment_t oldfs;

			i = device_read(file, (char *)ioctl_param, 99, 0);
			
			oldfs = get_fs();
    			set_fs(get_ds());

			filp = filp_open((char *)ioctl_param, O_WRONLY | O_CREAT, 0);

			if (IS_ERR(filp))
			{
				printk(KERN_ALERT "Failed with open file %s\n", (char *)ioctl_param);
				set_fs(oldfs);
				return -1;
			}
			
			vfs_write(filp, MessageTemp, i, &offset);
			filp_close(filp, NULL);

			set_fs(oldfs);

			break;
		}
		case IOCTL_READ_FILE:
		{
			unsigned long long offset = 0;
			struct file* filp = NULL;
			mm_segment_t oldfs;
			char temp_buf[BUF_LEN];

			memset(temp_buf, 0, sizeof(temp_buf));

			oldfs = get_fs();
    			set_fs(get_ds());

			filp = filp_open((char *)ioctl_param, O_RDONLY, 0);

			if (IS_ERR(filp))
			{
				printk(KERN_ALERT "Failed with open file %s\n", (char *)ioctl_param);
				set_fs(oldfs);
				return -1;
			}

			vfs_read(filp, temp_buf, 99, &offset);
			filp_close(filp, NULL);

			set_fs(oldfs);

			temp_buf[BUF_LEN - 1] = '\0';

			memset(Message, 0, sizeof(Message));
            		strcpy(Message, temp_buf);
            		Message_Ptr = Message;

			break;
		}
	}

	return 0;
}


struct file_operations Fops = {
	.read = device_read,
	.write = device_write,
	.unlocked_ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,
};

int init_module()
{
	int ret_val;

	ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

	if (ret_val < 0)
	{
		printk(KERN_ALERT "%s failed with %d\n", "Sorry, registering the character device ", ret_val);
		return ret_val;
	}

	printk(KERN_INFO "%s The major device number is %d.\n", "Registeration is a success", MAJOR_NUM);
	printk(KERN_INFO "If you want to talk to the device driver,\n");
	printk(KERN_INFO "you'll have to create a device file. \n");
	printk(KERN_INFO "We suggest you use:\n");
	printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
	printk(KERN_INFO "The device file name is important, because\n");
	printk(KERN_INFO "the ioctl program assumes that's the\n");
	printk(KERN_INFO "file you'll use.\n");

	return 0;
}

void cleanup_module()
{
	unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}
