# Linux-kernel
A collection of my notes and resources for learning kernel exploitation.

# Dirty
1. [Dirty Pagetable](https://yanglingxi1993.github.io/dirty_pagetable/dirty_pagetable.html)
2. [Dirty Pagetable CTF chal](https://ptr-yudai.hatenablog.com/entry/2023/12/08/093606)
3. [DirtyCred Remastered](https://exploiter.dev/blog/2022/CVE-2022-2602.html)
4. DirtyPipe:
   1. Official author: https://dirtypipe.cm4all.com
   2. https://www.aquasec.com/blog/deep-analysis-of-the-dirty-pipe-vulnerability/
   3. https://vsociety.medium.com/the-de-vinci-of-dirtypipe-local-privilege-escalation-cve-2022-0847-e0e391d2727b
  

# Netfilter/nftables
1. [CVE-2023-0179](https://betrusted.it/blog/64-bytes-and-a-rop-chain-part-1/)
   * [Part 2](https://betrusted.it/blog/64-bytes-and-a-rop-chain-part-2/)
2. `nftables` and **CVE-2022-1015** [link](https://blog.dbouman.nl/2022/04/02/How-The-Tables-Have-Turned-CVE-2022-1015-1016/#5-cve-2022-1016)
  

# File descriptors
<img width="677" alt="image" src="https://github.com/wechicken456/Linux-kernel/assets/55309735/704a3479-c4e6-42b1-a9dd-790ddf884cca">

Each userspace file descriptor is a reference to the Open File Table in the kernel. The kernel must keep track of these references to be able to know when any given file structure is no longer used and can be freed; that is done using the ```f_count``` field.

# SCM_RIGHTS
**Unix-domain sockets** are use for inter-process communication. Processes can pass a fd to another through **SCM_RIGHTS** messages:

1. Create a new reference to the file struct behind the sending file descriptor through ```sendmsg()``` syscall (implemented as ```unix_stream_sendmsg``` in the kernel).
2. *Queue* the reference until recevier accepts the connection.
3. *Receiver* decrements the reference to file.

=> If both sides close their sockets before accepting the **inflight** references, they will lose the only visible references to the file structs.

=> Those file structures will have a permanently elevated reference ```count``` and can never be freed. 

Kernel mitigations:
1. When a ```file``` structure corresponding to a Unix-domain socket gains a reference from an ```SCM_RIGHTS``` datagram, the ```inflight``` field of the corresponding ```unix_sock``` structure is incremented.
2. When the other side accepts that reference, ```inflight``` is decremented.

# io_uring
[Authors' paper](https://kernel.dk/io_uring.pdf).

[Examples of usage](https://unixism.net/2020/04/io-uring-by-example-part-1-introduction/).


# Linux namespaces
Used in Docker containerization. Can isolate different types of resources such as mount namespace, process namespace, network namespace, etc.

To create a namespace, use the command ```unshare```: 
https://www.man7.org/linux/man-pages/man1/unshare.1.html

Then, to create a private root fs, use ```pivot_root```:
https://www.man7.org/linux/man-pages/man2/pivot_root.2.html

A process can belong to different namespaces.

```unshare``` and ```pivot_root``` does **NOT** automatically change the current working dir to the pivoted_root dir!

To enter an already created namnespace, use the command ```nsenter``` (wrapper for [sys_setns](https://www.man7.org/linux/man-pages/man2/setns.2.html)):
https://www.man7.org/linux/man-pages/man1/nsenter.1.html


## NOTES & TRICKS
```unshare``` and ```pivot_root``` does **NOT** automatically change the current working dir to the pivoted_root dir!


Any file descriptor opened **BEFORE** entering the pivoted_root/namespace will persist.

If we have a reference to any namespace outside of the container (such as ```/proc/pid of process outside/mnt```), we can use ```setns(fd, 0)``` to switch to that namespace.


# Symbolic links
To overwrite an existing symbolic link for a **directory**, use the ```-n``` option in ```ln```: https://superuser.com/questions/645842/how-to-overwrite-a-symbolic-link-of-a-directory

# Devices and Drivers
Devices are located in `/dev`, operations performed on these files will invoke the corresponding drivers to communicate with the corresponding hardware.

```
$ ls -l /dev/hda[1-3]
brw-rw----  1 root  disk  3, 1 Jul  5  2000 /dev/hda1
brw-rw----  1 root  disk  3, 2 Jul  5  2000 /dev/hda2
brw-rw----  1 root  disk  3, 3 Jul  5  2000 /dev/hda3
```
The first letter in the permission column, `b`, stands for block devices, while `c` stands for character devices.

In the number columns separated by comma, the first one - `3` - is the device's major number, 2nd one is the minor version. Each *driver* gets a **unique major number**.
   - Major number: the driver the hardware belongs to.
   - Minor number: which hardware resource the driver is controlling (could be physically the same piece).
=> Same major = controlled by the same driver. Different minor = different pieces of "hardware". The term "hardware" can mean something very abstract...

<ins>**Note**</ins>: When a device file is accessed, the kernel uses major number -> find appropriate driver -> driver, not the kernel, decides what to do with the minor version.


## Block devices
1. Buffered.
2. Inputs and Outputs in fixed blocks of bytes.

## Character devices
1. Use any number of bytes.
2. 
