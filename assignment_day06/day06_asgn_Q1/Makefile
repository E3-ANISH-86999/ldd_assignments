obj-m = pseudochar_ioctl.o

pseudochar_ioctl.ko: pseudochar_ioctl.c
	make -C /lib/modules/$$(uname -r)/build M=$$(pwd) modules

clean:
	make -C /lib/modules/$$(uname -r)/build M=$$(pwd) clean

.PHONY: clean


