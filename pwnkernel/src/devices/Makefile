# add more modules here!
obj-m += chardev.o 
obj-m += charproc.o
obj-m += hello_sysfs.o
KERNEL_VERSION=6.2.16

# use this instead of $PWD because I call `make` to this Makefile from other directories using `make -C this_dir`, but then $PWD doesn't change to this directory.
# use $realpath to trim the trailing forwarad slash
makeFileDir := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
#$(info ================================================================)
#$(info $(MAKEFILE_LIST))
#$(info $(lastword $(MAKEFILE_LIST)))
#$(info $(makeFileDir))

all: 
# make -C /usr/src/linux-headers-6.8.0-45-generic M=$(PWD) modules
# replace the line above with the below line if not installing for the running kernel
	make -C ../../linux-$(KERNEL_VERSION) M=$(makeFileDir) modules
clean:

# make -C /usr/src/linux-headers-6.8.0-45-generic M=$(PWD) clean
# replace the line above with the below line if not installing for the running kernel
	make -C ../../linux-$(KERNEL_VERSION) M=$(makeFileDir) clean
