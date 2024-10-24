# add more modules here!
obj-m += hello-1.o 
obj-m += hello-2.o
obj-m += hello-3.o
KERNEL_VERSION=6.2.16

all: 
	echo $(PWD)
	echo $(OBJECTS)
	make -C /usr/src/linux-headers-6.8.0-45-generic M=$(PWD) modules
# replace the line above with this line if not installing for the running kernel
# make -C ../pwnkernel/linux-6.2.16  M=$(PWD) modules

clean:

	make -C /usr/src/linux-headers-6.8.0-45-generic M=$(PWD) clean
# replace the line above with this line if not installing for the running kernel
# make -C ../pwnkernel/linux-6.2.16 M=$(PWD) clean
