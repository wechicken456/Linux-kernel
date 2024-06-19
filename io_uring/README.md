# Completion queue:

```
struct io_uring_cqe {
    __u64  user_data;  /* sqe->user_data submission passed back */
    __s32  res;    /* result code for this event */
    __u32  flags;
};
```
- ```user_data```: passed from SQ to CQ. Used to identify the SQE that corresponds to this CQE. This is because the orders of completion of events might change because they take different times to finish.
- ```res```: return value of the syscall(think the number of read bytes returned by ```read()```).

# Submission queue:
```
struct io_uring_sqe {
  __u8  opcode;    /* type of operation for this sqe */
  __u8  flags;    /* IOSQE_ flags */
  __u16  ioprio;    /* ioprio for the request */
  __s32  fd;    /* file descriptor to do IO on */
  __u64  off;    /* offset into file */
  __u64  addr;    /* pointer to buffer or iovecs */
  __u32  len;    /* buffer size or number of iovecs */
  union {
    __kernel_rwf_t  rw_flags;
    __u32    fsync_flags;
    __u16    poll_events;
    __u32    sync_range_flags;
    __u32    msg_flags;
  };
  __u64  user_data;  /* data to be passed back at completion time */
  union {
    __u16  buf_index;  /* index into fixed buffers, if used */
    __u64  __pad2[3];
  };
};
```

# io_uring interface

## Initial setup
On a successful call to *```io_uring_setup(u32 entries, struct io_uring_params)```*, the kernel will return a file descriptor that is used to refer to this ```io_uring``` instance:
```
struct io_uring_params {
    __u32 sq_entries;
    __u32 cq_entries;
    __u32 flags;
    __u32 sq_thread_cpu;
    __u32 sq_thread_idle;
    __u32 features;
    __u32 resv[4];
    struct io_sqring_offsets sq_off;
    struct io_cqring_offsets cq_off;
};
```

***```sq_off```***: offsets of the various ring members.

***```sq_entries```***: number of sqes.

The sqe and cqe structures are shared by the kernel and the application => eliminate the need of copying data during I/O.
Therefore, application needs a way to gain access to this memory. This is done through ```mmap```'ing it into the application memory space. 


The ```io_sqring_offsets``` struct of ```sq_off```:
```
struct io_sqring_offsets {  
    __u32 head;              // offset of ring head
    __u32 tail;              // offset of ring tail
    __u32 ring_mask;         // ring size mask
    __u32 ring_entries;      // entries in ring
    __u32 flags;             // ring flags
    __u32 dropped;           // number of sqes not submitted
    __u32 array;             // sqe index array
    __u32 resv[3];
};
```

To ```mmap``` the **SQ** and access the SQ head:
```
ptr = mmap(0, sq_off.array + sq_entries * sizeof(__u32),
           PROT_READ|PROT_WRITE, MAP_SHARED|MAP_POPULATE,
           ring_fd, IORING_OFF_SQ_RING);

head = ptr + sq_off.head
```

Something I don't understand: "While the completion queue ring directly indexes the shared array of CQEs, the submission ring has an indirection array in between. The submission side ring buffer is an index into this array, which in turn contains the index into the SQEs. This is useful for certain applications that embed submission requests inside of internal data structures."

Then you have to ```mmap``` the **SQE** and **CQ** separately as well.

## Enter io_uring
To tell the kernel that there is work to consumed, the application calls: 
```
io_uring_enter(unsigned int fd, unsigned int to_submit, unsigned int min_complete, unsigned int flags, sigset_t sig)
```
- ```fd```: io_uring instance returned by ```io_uring_setup(...)```.
- ```to_submit```: number of ***SQE***s ready to be consumed.
- ```min_complete```: wait for completion of that amount of tasks.

=> *submit* and *wait* in 1 single syscall!!!

## Reading entries from CQ
Whenever a new event is posted by the kernel to the CQ ring, it updates the **tail** associated with it. When the application consumes an entry, it updates the ***head***. 
Hence, if tail != head, the application knows that it events available for consumption:
```
unsigned head;
head = cqring->head;
read_barrier(); /* ensure previous writes are visible */
if (head != cqring->tail) {
    /* There is data available in the ring buffer */
    struct io_uring_cqe *cqe;
    unsigned index;
    index = head & (cqring->mask);
    cqe = &cqring->cqes[index];
    /* process completed cqe here */
     ...
    /* we've now consumed this entry */
    head++;
}
cqring->head = head;
write_barrier();
```

We can use masking here to retrieve the index of the head because the ```cqring->mask``` (size of the CQE array) is of powers of 2. From the original [paper](https://kernel.dk/io_uring.pdf): 
> The ring counters themselves are free flowing 32-bit integers, and rely on natural wrapping when the number of completed events exceed the capacity of the ring. One advantage of this approach is that we can utilize the full size of the ring without having to manage a "ring is full" flag on the side, which would have complicated the management of the ring. With that, it also follows that the ring must be a power of 2 in size.


