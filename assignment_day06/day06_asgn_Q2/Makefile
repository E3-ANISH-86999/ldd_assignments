obj-m = pchar_multi_device_ioctl.o 

pchar_multi_device_ioctl.ko: pchar_multi_device_ioctl.c
	make -C /lib/modules/$$(uname -r)/build M=$$(pwd) modules

clean:
	make -C /lib/modules/$$(uname -r)/build M=$$(pwd) clean

.PHONY: clean
