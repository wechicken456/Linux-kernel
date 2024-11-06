# Linux-kernel
A collection of my notes and resources for learning kernel exploitation.
More notes [here](https://docs.google.com/document/d/1d-h7NJf2IDDT5ifg9XitlXnJHNWhDX12U-yUVZCRowM/edit?usp=sharing) and [here](https://www.notion.so/Kernel-exploit-notebook-79acde7c62bb4c49ac21641b507d5c77?pvs=4).

## Dirty
1. [Dirty Pagetable](https://yanglingxi1993.github.io/dirty_pagetable/dirty_pagetable.html)
2. [Dirty Pagetable CTF chal](https://ptr-yudai.hatenablog.com/entry/2023/12/08/093606)
3. [DirtyCred Remastered](https://exploiter.dev/blog/2022/CVE-2022-2602.html)
4. DirtyPipe:
   1. Official author: https://dirtypipe.cm4all.com
   2. https://www.aquasec.com/blog/deep-analysis-of-the-dirty-pipe-vulnerability/
   3. https://vsociety.medium.com/the-de-vinci-of-dirtypipe-local-privilege-escalation-cve-2022-0847-e0e391d2727b
  

## Netfilter/nftables
1. [CVE-2023-0179](https://betrusted.it/blog/64-bytes-and-a-rop-chain-part-1/)
   * [Part 2](https://betrusted.it/blog/64-bytes-and-a-rop-chain-part-2/)
2. `nftables` and **CVE-2022-1015** [link](https://blog.dbouman.nl/2022/04/02/How-The-Tables-Have-Turned-CVE-2022-1015-1016/#5-cve-2022-1016)
  

## File descriptors
<img width="677" alt="image" src="https://github.com/wechicken456/Linux-kernel/assets/55309735/704a3479-c4e6-42b1-a9dd-790ddf884cca">

Each userspace file descriptor is a reference to the Open File Table in the kernel. The kernel must keep track of these references to be able to know when any given file structure is no longer used and can be freed; that is done using the ```f_count``` field.

## SCM_RIGHTS
**Unix-domain sockets** are use for inter-process communication. Processes can pass a fd to another through **SCM_RIGHTS** messages:

1. Create a new reference to the file struct behind the sending file descriptor through ```sendmsg()``` syscall (implemented as ```unix_stream_sendmsg``` in the kernel).
2. *Queue* the reference until recevier accepts the connection.
3. *Receiver* decrements the reference to file.

=> If both sides close their sockets before accepting the **inflight** references, they will lose the only visible references to the file structs.

=> Those file structures will have a permanently elevated reference ```count``` and can never be freed. 

Kernel mitigations:
1. When a ```file``` structure corresponding to a Unix-domain socket gains a reference from an ```SCM_RIGHTS``` datagram, the ```inflight``` field of the corresponding ```unix_sock``` structure is incremented.
2. When the other side accepts that reference, ```inflight``` is decremented.

## io_uring
[Authors' paper](https://kernel.dk/io_uring.pdf).

(Examples of usage)[https://unixism.net/2020/04/io-uring-by-example-part-1-introduction/].
