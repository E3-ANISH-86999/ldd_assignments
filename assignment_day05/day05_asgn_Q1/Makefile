obj-m = SplitModule.o
SplitModule-objs = split_module1.o split_module2.o

SplitModule.ko: split_module1.c split_module2.c
	make -C /lib/modules/$$(uname -r)/build M=$$(pwd) modules

clean:
	make -C /lib/modules/$$(uname -r)/build M=$$(pwd) clean

.PHONY: clean


