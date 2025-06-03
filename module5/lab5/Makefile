obj-m += netlink.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc-14 -o user user.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -r user

kernel:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

user:
	gcc-14 -o user user.c

cleankernel:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

cleanuser:
	rm -r user