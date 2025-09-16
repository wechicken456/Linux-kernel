/*
 * In this example, we process the files one after the other. 
 * We are not going to do any parallel I/O since this is a simple example designed mainly to get an understanding of io_uring. 
 * To this end, we set the queue depth to just one.
 * References:
 *      + io_uring API: https://nick-black.com/dankwiki/index.php/Io_uring
 *      + io_uring_enter, io_uring_params, io_sqring_offsets: https://unixism.net/loti/ref-iouring/io_uring_setup.html
 *      + 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
/* If your compilation fails because the header file below is missing,
 * your kernel is probably too old to support io_uring.
 * */
#include <linux/io_uring.h>
#include <liburing.h>

#define QUEUE_DEPTH 1
#define BLOCK_SZ 1024

/* Tell the COMPILER to not reorder memory access. 
 * https://stackoverflow.com/questions/14950614/working-of-asm-volatile-memory
 */
#define read_barrier()  __asm__ __volatile__("":::"memory")
#define write_barrier() __asm__ __volatile__("":::"memory")

/**
 * For references:
struct io_sqring_offsets {
    __u32 head;
    __u32 tail;
    __u32 ring_mask;
    __u32 ring_entries;
    __u32 flags;
    __u32 dropped;
    __u32 array;
    __u32 resv[3];
};

struct io_cqring_offsets {
    __u32 head;
    __u32 tail;
    __u32 ring_mask;
    __u32 ring_entries;
    __u32 overflow;
    __u32 cqes;
    __u32 resv[4];
};

 */

/**
 * everything is a pointer, because as mentioned below during `cring_sz` and `sring_sz` initialization,
 * both SQ and CQ contain internal data structures, so we have to rely on io_sqring_offsets/io_cqring_offsets struct above
 * to get the information we want.
 * Check out struct io_uring: https://nick-black.com/dankwiki/index.php/Io_uring
 */ 
struct app_io_sq_ring {
    unsigned *head;
    unsigned *tail;
    unsigned *ring_mask;
    unsigned *ring_entries;
    unsigned *flags;
    unsigned *array;
};

struct app_io_cq_ring {
    unsigned *head;
    unsigned *tail;
    unsigned *ring_mask;
    unsigned *ring_entries;
    struct io_uring_cqe *cqes;
};

struct submitter {
    int ring_fd;
    struct app_io_sq_ring   sq_ring;
    struct io_uring_sqe     *sqes;
    struct app_io_cq_ring   cq_ring;
};

struct file_info {
    off_t file_sz;
    struct iovec iovecs[];      // for readv/writev
};


int app_setup_uring(struct submitter *s) {
    struct app_io_sq_ring *sring = &s->sq_ring;
    struct app_io_cq_ring *cring = &s->cq_ring;
    struct io_uring_params p;
    void *sq_ptr, *cq_ptr;

    memset(&p, 0, sizeof(p));   /* we're only given permission to set a few flags, but none of them are interesting rn. */

    s->ring_fd = (int)syscall(__NR_io_uring_setup, QUEUE_DEPTH, &p);
    if (s->ring_fd < 0) {
        perror("io_uring_setup");
        return 4;
    }

    /**
     * The submission queue is an internal data structure followed by an array of SQE descriptors. 
     * These descriptors are 32 bits each no matter the architecture, implying that they are indices into the SQE map, not pointers. 
     * The completion queue is an internal data structure, but is much simpler - it contains the CQEs within.
     */
    int sring_sz = p.sq_off.array + p.sq_entries * sizeof(unsigned);
    int cring_sz = p.cq_off.cqes + p.cq_entries * sizeof(struct io_uring_cqe);

    /**
     * If the IORING_FEAT_SINGLE_MMAP flag is set, the two SQ and CQ rings can be mapped with a single mmap(2) call. 
     * The SQEs must still be allocated separately. 
     */
    if (p.features & IORING_FEAT_SINGLE_MMAP) {
        if (cring_sz > sring_sz) {
            sring_sz = cring_sz;
        }
        cring_sz = sring_sz;
    }

    // MAP_POPULATE: prefault page tables for this mapping. For a file, tihs causes read-ahead to reduce blocking on page faults later.
    sq_ptr = mmap(0, sring_sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, s->ring_fd, IORING_OFF_SQ_RING);
    if (sq_ptr < 0) {
        perror("sring mmap");
        return 5;
    }
    if (p.features & IORING_FEAT_SINGLE_MMAP) {
        cq_ptr = sq_ptr;
    }
    else {
        cq_ptr = mmap(0, cring_sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, s->ring_fd, IORING_OFF_CQ_RING);
        if (cq_ptr < 0) {
            perror("cring mmap");
            return 6;
        }
    }

    /**
     * Save useful fields in a wrapper data structure for easiers references
     */
    sring->head = sq_ptr + p.sq_off.head;
    sring->tail = sq_ptr + p.sq_off.tail;
    sring->array = sq_ptr + p.sq_off.array;
    sring->ring_entries = sq_ptr + p.sq_off.ring_entries;
    sring->ring_mask = sq_ptr + p.sq_off.ring_mask;
    sring->flags = sq_ptr + p.sq_off.flags;

    /**
     * now map the submission queue entries.
     */
    s->sqes = mmap(0, p.sq_entries * sizeof(struct io_uring_sqe), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, s->ring_fd, IORING_OFF_SQES);
    if (!s->sqes) {
        perror("sqes mmap");
        return 7;
    }

    cring->head = cq_ptr + p.cq_off.head;
    cring->tail = cq_ptr + p.cq_off.tail;
    cring->cqes = cq_ptr + p.cq_off.cqes;
    cring->ring_entries = cq_ptr + p.cq_off.ring_entries;
    cring->ring_mask = cq_ptr + p.cq_off.ring_mask;
    
    return 0;
}

off_t get_file_size(int fd) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        return -1;
    }
    if (S_ISBLK(st.st_mode)) {      // block devices
        unsigned long long nbytes;
        if (ioctl(fd, BLKGETSIZE64, &nbytes) != 0) {
            perror("ioctl");
            return -1;
        }
        return nbytes;
    }
    else if (S_ISREG(st.st_mode)) {           // regular files
        return st.st_size;
    }

    return -1;
}

/*
 * We're using buffered output here to be efficient.
 */
void print_to_console(char *buf, unsigned long len) {
    while (len--) {
        fputc(*buf++, stdout);
    }
}

int submit_to_sq(char *filename, struct submitter *s) {
    struct app_io_sq_ring *sring = &s->sq_ring;
    struct file_info *fi;

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open input file");
        return 8;
    }

    unsigned index = 0, current_block = 0, tail = 0, next_tail = 0;
    off_t file_sz = get_file_size(fd);
    if (file_sz < 0)
        return 10;

    off_t bytes_remaining = file_sz;
    int blocks = (int) file_sz / BLOCK_SZ;
    if (file_sz % BLOCK_SZ) blocks++;

    fi = malloc(sizeof(*fi) + sizeof(struct iovec) * blocks);
    if (!fi) {
        fprintf(stderr, "Unable to allocate memory\n");
        return 11;
    }
    fi->file_sz = file_sz;

    /*
     * For each block of the file we need to read, we allocate an iovec struct
     * which is indexed into the iovecs array. This array is passed in as part
     * of the submission. If you don't understand this, then you need to look
     * up how the readv() and writev() system calls work.
     * */
    while (bytes_remaining) {
        off_t bytes_to_read = bytes_remaining;
        if (bytes_to_read > BLOCK_SZ)
            bytes_to_read = BLOCK_SZ;
        fi->iovecs[current_block].iov_len = bytes_to_read;
        void *buf;
        if( posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ)) {
            perror("posix_memalign");
            return 12;
        }
        fi->iovecs[current_block].iov_base = buf;
        current_block++;
        bytes_remaining -= bytes_to_read;
    }
    /* Add our submission queue entry to the tail of the SQE ring buffer */
    next_tail = tail = *sring->tail;
    next_tail++;
    read_barrier();
    index = tail & *s->sq_ring.ring_mask;
    struct io_uring_sqe *sqe = &s->sqes[index];
    sqe->fd = fd;
    sqe->flags = 0;
    sqe->opcode = IORING_OP_READV;
    sqe->addr = (unsigned long) fi->iovecs;
    sqe->len = blocks;
    sqe->off = 0;
    sqe->user_data = (unsigned long long) fi;
    sring->array[index] = index;
    tail = next_tail;

    /* Update the tail so the kernel can see it. */
    if(*sring->tail != tail) {
        *sring->tail = tail;
        write_barrier();
    }
    /*
     * Tell the kernel we have submitted events with the io_uring_enter() system
     * call. We also pass in the IOURING_ENTER_GETEVENTS flag which causes the
     * io_uring_enter() call to wait until min_complete events (the 3rd param)
     * complete.
     * */
    int ret =  (int) syscall(__NR_io_uring_enter, s->ring_fd, 1,1,
                                IORING_ENTER_GETEVENTS, NULL, 0);
    if(ret < 0) {
        perror("io_uring_enter");
        return 13;
    }
    return 0;
}

int read_from_cq(struct submitter *s) {
    struct file_info *fi;
    struct app_io_cq_ring *cring = &s->cq_ring;
    struct io_uring_cqe *cqe;
    unsigned head, reaped = 0;
    head = *cring->head;
    do {
        read_barrier();
        /*
         * Remember, this is a ring buffer. If head == tail, it means that the
         * buffer is empty.
         * */
        if (head == *cring->tail)
            break;
        /* Get the entry */
        cqe = &cring->cqes[head & *s->cq_ring.ring_mask];
        fi = (struct file_info*) cqe->user_data;
        if (cqe->res < 0) {
            fprintf(stderr, "Error: %s\n", strerror(abs(cqe->res)));
            return 9;
        }
        int blocks = (int) fi->file_sz / BLOCK_SZ;
        if (fi->file_sz % BLOCK_SZ) blocks++;
        for (int i = 0; i < blocks; i++) {
            print_to_console(fi->iovecs[i].iov_base, fi->iovecs[i].iov_len);
        }
        head++;
    } while (1);
    *cring->head = head;
    write_barrier();
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
        return 1;
    }

    struct submitter *s;;
    s = malloc(sizeof(struct submitter));
    if (!s) {
        fprintf(stderr, "Failed to mallic submitter...\n");
        return 2;
    }
    memset(s, 0, sizeof(*s));

    if (app_setup_uring(s)) {
        fprintf(stderr, "Failed to setup io_uring...");
        return 3;
    }

    for (int i = 1; i < argc; i++) {
        int ret = submit_to_sq(argv[i], s);
        if(ret) {
            fprintf(stderr, "submit_to_sq(%s, s) failed with return value %d. Terminating...\n", argv[i], ret);
            return 4;
        }
        read_from_cq(s);
    }    
}