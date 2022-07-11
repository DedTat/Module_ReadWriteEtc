#include "chardev.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

void ioctl_set_msg(int file_desc, char *message)
{
	int ret_val = ioctl(file_desc, IOCTL_SET_MSG, message);

	if (ret_val < 0)
	{
		printf("ioctl_set_msg failed: %d\n", ret_val);
		exit(-1);
	}
}


void ioctl_get_msg(int file_desc)
{
	char message[100];
	int ret_val = ioctl(file_desc, IOCTL_GET_MSG, message);

	if (ret_val < 0)
	{
        	printf("ioctl_get_msg failed: %d\n", ret_val);
        	exit(-1);
	}

	printf("*** get_msg message begin ***\n");
	printf("%s", message);
	printf("*** get_msg message end ***\n");
}


void ioctl_get_nth_byte(int file_desc)
{
	int i = 0;
	char c;

	printf("*** get_nth_byte message begin ***\n");

	do
	{
		c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, i++);

		if (c < 0)
		{
        		printf ("ioctl_get_nth_byte failed at the %d'th byte:\n", i);
        		exit(-1);
        	}

        	putchar(c);
    	} while (c != 0);

	printf("*** get_nth_byte message end ***\n");
}

void ioctl_write_file(int file_desc, char *file_write_desc)
{
	int ret_val = ioctl(file_desc, IOCTL_WRITE_FILE, file_write_desc);

	if (ret_val < 0)
	{
		printf("ioctl_write_file failed: %d\n", ret_val);
		exit(-1);
	}
}

void ioctl_read_file(int file_desc, char *file_read_desc)
{
	int ret_val = ioctl(file_desc, IOCTL_READ_FILE, file_read_desc);

	if (ret_val < 0)
	{
        	printf("ioctl_read_file failed: %d\n", ret_val);
        	exit(-1);
	}
}

void main()
{
	char *fileName	  = "file";

	char *msg         = "Hi all\n";
	char *msgDelete   = "delete\n";
 	char *msgForward  = "direction forward\n";
 	char *msgBack     = "direction back\n";

	int file_desc = open(DEVICE_FILE_NAME, 0);

	if (file_desc < 0)
	{
		printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}

	ioctl_get_nth_byte(file_desc);

	ioctl_set_msg(file_desc, msg);
	ioctl_get_msg(file_desc);

	ioctl_set_msg(file_desc, msg);
	ioctl_write_file(file_desc, fileName);

	ioctl_read_file(file_desc, fileName);
	ioctl_get_msg(file_desc);

  	ioctl_set_msg(file_desc, msgDelete);
  	ioctl_get_msg(file_desc);

  	ioctl_set_msg(file_desc, msgBack);
  	ioctl_set_msg(file_desc, msg);
  	ioctl_get_msg(file_desc);

	ioctl_set_msg(file_desc, msgForward);
	ioctl_set_msg(file_desc, msgDelete);
	ioctl_read_file(file_desc, fileName);
  	ioctl_get_msg(file_desc);

	close(file_desc);
}


