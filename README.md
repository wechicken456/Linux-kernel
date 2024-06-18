# Linux-kernel
A collection of my notes and resources for learning kernel exploitation.

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
  

## file descriptors and io_uring
<img width="677" alt="image" src="https://github.com/wechicken456/Linux-kernel/assets/55309735/704a3479-c4e6-42b1-a9dd-790ddf884cca">

Each userspace file descriptor is a reference to the Open File Table in the kernel. The kernel must keep track of these references to be able to know when any given file structure is no longer used and can be freed; that is done using the ```f_count``` field.
