#!/bin/bash -e

export KERNEL_VERSION=6.2.16
export BUSYBOX_VERSION=1.36.0

#
# dependencies
#

#echo "[+] Checking / installing dependencies..."
#sudo apt-get -q update
#sudo apt-get -q install -y bc bison flex libelf-dev cpio build-essential libssl-dev qemu-system-x86

#
# linux kernel
#

if [ ! -d "linux-$KERNEL_VERSION" ]; then
	echo "[+] Downloading kernel..."
	wget -q -c https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/snapshot/linux-$KERNEL_VERSION.tar.gz
	[ -e linux-$KERNEL_VERSION ] || tar xzf linux-$KERNEL_VERSION.tar.gz
else
	echo "[+] Found kernel source dir in current dir. Using it instead of downloading a new one..."
fi

read -p  "Rebuild kernel? [Y/N] " rebuild
if [[ "$rebuild" =~ [yY] ]]; then

	echo "[+] Building kernel..."
	make -C linux-$KERNEL_VERSION defconfig
	echo "CONFIG_NET_9P=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_NET_9P_DEBUG=n" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_9P_FS=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_9P_FS_POSIX_ACL=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_9P_FS_SECURITY=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_NET_9P_VIRTIO=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_VIRTIO_PCI=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_VIRTIO_BLK=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_VIRTIO_BLK_SCSI=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_VIRTIO_NET=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_VIRTIO_CONSOLE=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_HW_RANDOM_VIRTIO=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_DRM_VIRTIO_GPU=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_VIRTIO_PCI_LEGACY=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_VIRTIO_BALLOON=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_VIRTIO_INPUT=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_CRYPTO_DEV_VIRTIO=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_BALLOON_COMPACTION=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_PCI=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_PCI_HOST_GENERIC=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_GDB_SCRIPTS=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_DEBUG_INFO=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_DEBUG_INFO_REDUCED=n" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_DEBUG_INFO_SPLIT=n" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_DEBUG_FS=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_DEBUG_INFO_DWARF4=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_DEBUG_INFO_BTF=y" >> linux-$KERNEL_VERSION/.config
	echo "CONFIG_FRAME_POINTER=y" >> linux-$KERNEL_VERSION/.config

	sed -i 'N;s/WARN("missing symbol table");\n\t\treturn -1;/\n\t\treturn 0;\n\t\t\/\/ A missing symbol table is actually possible if its an empty .o file.  This can happen for thunk_64.o./g' linux-$KERNEL_VERSION/tools/objtool/elf.c

	sed -i 's/unsigned long __force_order/\/\/ unsigned long __force_order/g' linux-$KERNEL_VERSION/arch/x86/boot/compressed/pgtable_64.c

	make -C linux-$KERNEL_VERSION -j16 bzImage
fi

#
# Busybox
#

if [ ! -d busybox-$BUSYBOX_VERSION ]; then
	echo "[+] Downloading busybox..."
	wget -q -c https://busybox.net/downloads/busybox-$BUSYBOX_VERSION.tar.bz2
	[ -e busybox-$BUSYBOX_VERSION ] || tar xjf busybox-$BUSYBOX_VERSION.tar.bz2

	echo "[+] Building busybox..."
	make -C busybox-$BUSYBOX_VERSION defconfig
	sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/g' busybox-$BUSYBOX_VERSION/.config
	make -C busybox-$BUSYBOX_VERSION -j16
	make -C busybox-$BUSYBOX_VERSION install
else
	echo "[+] Found busybox-$BUSYBOX_VERSION directory. Using it instead of rebuilding..." 
fi

#
# filesystem
#

echo "[+] Building filesystem..."
cd fs
mkdir -p bin sbin etc proc sys usr/bin usr/sbin root home/ctf home/modules
cd ..
cp -a busybox-$BUSYBOX_VERSION/_install/* fs
cp ./qemu_init fs/init
chmod +x fs/init

#
# modules
# NOTE: run `make` in all subdirctories of src/, then copy the .ko files in src/ to fs/

echo "[+] Building modules..."
fs_copy_dir=$PWD/fs/home/modules
linux_dir=$PWD/linux-$KERNEL_VERSION
cd src
for dir in $(find $PWD/ -type d); do
	make -C $dir || continue
	cp $dir/*.ko $fs_copy_dir || continue
# needs these modules to be present in the linux-X-X dir as well in order for gdb kernel scripts to load their debug info.
	cp $dir/*.ko $linux_dir || continue 
done

