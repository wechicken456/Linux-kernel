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

#include <liburing.h>

#define QUEUE_DEPTH 1
#define BLOCK_SZ 1024

struct file_info {
    off_t file_sz;
    struct iovec iovecs[];      // for readv/writev
};

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

int get_completion_and_print(struct io_uring *ring) {
    struct io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(ring, &cqe);
    if (ret < 0) {
        fprintf(stderr, "io_uring_wait_cqe");
        return 1;
    }
    if (cqe->res < 0) {
        fprintf(stderr, "Async readv failed...");
        return 2;
    }
    struct file_info *fi = io_uring_cqe_get_data(cqe);
    int blocks = fi->file_sz / BLOCK_SZ;
    if (fi->file_sz % BLOCK_SZ) blocks++;
    for (int i = 0; i < blocks; i++) {
        print_to_console(fi->iovecs[i].iov_base, fi->iovecs[i].iov_len);
    }
    io_uring_cqe_seen(ring, cqe);
    return 0;
}

int submit_read_request(const char *filename, struct io_uring *ring) {
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        perror("open");
        return 1;
    }
    off_t file_sz = get_file_size(file_fd);
    if (file_sz < 0) {
        return 2;
    }
    off_t bytes_remaining = file_sz;
    off_t offset = 0;
    int current_block = 0;
    int blocks = file_sz / BLOCK_SZ;
    if (file_sz % BLOCK_SZ) blocks++;

    struct file_info *fi = malloc(sizeof(*fi) + sizeof(struct iovec)*blocks);

    /*
     * For each block of the file we need to read, we allocate an iovec struct
     * which is indexed into the iovecs array. This array is passed in as part
     * of the submission. If you don't understand this, then you need to look
     * up how the readv() and writev() system calls work.
     * */
    while (bytes_remaining) {
        off_t bytes_to_read = bytes_remaining;
        if (bytes_to_read > BLOCK_SZ) bytes_to_read = BLOCK_SZ;

        offset += bytes_to_read;
        fi->iovecs[current_block].iov_len = bytes_to_read;
        
        void *buf;
        if (posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ)) {
            perror("posix_memalign");
            return 3;
        }

        fi->iovecs[current_block].iov_base = buf;
        current_block++;
        bytes_remaining -= bytes_to_read;
    }
    fi->file_sz = file_sz;

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    /* Setup a readv operation */
    io_uring_prep_readv(sqe, file_fd, fi->iovecs, blocks, 0);
    /* Set user data */
    io_uring_sqe_set_data(sqe, fi);
    /* finally submit */
    io_uring_submit(ring);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
        return 1;
    }

    struct io_uring ring;
    io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    
    for (int i = 1; i < argc; i++) {
        int ret = submit_read_request(argv[i], &ring);
        if (ret) {
            fprintf(stderr, "Error reading file: %s\n", argv[i]);
            return 1;
        }
        get_completion_and_print(&ring);
    }

    /* MUST call the clean-up function. */
    io_uring_queue_exit(&ring);
    return 0;
}

