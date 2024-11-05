#!/bin/bash

#
# build root fs
#
pushd fs
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz
popd

#
# launch
#
/usr/bin/qemu-system-x86_64 \
	-m 4096 \
	-kernel linux-6.2.16/arch/x86/boot/bzImage \
	-initrd $PWD/initramfs.cpio.gz \
	-fsdev local,security_model=passthrough,id=fsdev0,path=/home/pwnphofun/Code/programming/kernel/custom_modules \
	-device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare \
	-nographic \
	-net nic \
	-net user,hostfwd=tcp::2222-:22 \
	-serial mon:stdio \
	-s \
	-append "console=ttyS0 nokaslr nopti nosmep nosmap quiet panic=1"
