obj-m += chardev.o

all:
	make -C /lib/modules/4.8.0-49-generic/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/4.8.0-49-generic/build M=$(shell pwd) clean
