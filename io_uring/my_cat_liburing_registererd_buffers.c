#include <fcntl.h>
#include <linux/fs.h>
#include <linux/io_uring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

#include <liburing.h>

#define QUEUE_DEPTH 2
#define BLOCK_SZ 4096

struct iovec iovecs[UIO_MAXIOV];

off_t get_file_size(int fd) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        return -1;
    }
    if (S_ISBLK(st.st_mode)) { // block devices
        unsigned long long nbytes;
        if (ioctl(fd, BLKGETSIZE64, &nbytes) != 0) {
            perror("ioctl");
            return -1;
        }
        return nbytes;
    } else if (S_ISREG(st.st_mode)) { // regular files
        return st.st_size;
    }

    return -1;
}

void print_sq_poll_kernel_thread_status() {

    if (system("ps --ppid 2 | grep io_uring-sq") == 0)
        printf("Kernel thread io_uring-sq found running...\n");
    else
        printf("Kernel thread io_uring-sq is not running.\n");
}

int read_and_print_file(const char *filename, struct io_uring *ring) {
    int fds[2];
    int file_fd = open(filename, O_RDONLY);
    fds[0] = file_fd;
    fds[1] = STDOUT_FILENO;

    if (file_fd < 0) {
        perror("open");
        return 1;
    }

    // must register file descriptors before any I/O with io_uring when we use
    // submission queue polling: https://unixism.net/loti/tutorial/sq_poll.html
    int ret = io_uring_register_files(ring, fds, 2);
    if (ret) {
        fprintf(stderr, "Error registering files: %s\n", strerror(-ret));
        return 1;
    }

    off_t file_sz = get_file_size(file_fd);
    if (file_sz < 0) {
        return 2;
    }

    int inflight_count = 0; // number of entries still in the io_uring queue
    int iov_idx = 0;
    off_t file_offset = 0;
    while (file_offset < file_sz || inflight_count) {
        // Fill the submission queue as long as there's a slot
        // Note: inflight_count tracks both reads AND writes, so we need
        // to ensure we don't exceed QUEUE_DEPTH * 2 to prevent buffer reuse
        while (file_offset < file_sz && inflight_count == 0) {
            struct io_uring_sqe *read_sqe = io_uring_get_sqe(ring);
            struct io_uring_sqe *write_sqe = io_uring_get_sqe(ring);
            if (!read_sqe || !write_sqe)
                break;

            off_t bytes_to_read = file_sz - file_offset;
            unsigned num_blocks = 0;
            if (bytes_to_read > UIO_MAXIOV * BLOCK_SZ) {
                bytes_to_read = UIO_MAXIOV * BLOCK_SZ;
            }
            num_blocks =
                bytes_to_read / BLOCK_SZ + (bytes_to_read % BLOCK_SZ != 0);

            io_uring_prep_readv(read_sqe, 0, iovecs, num_blocks, file_offset);
            io_uring_sqe_set_data(read_sqe, NULL);
            // MUST set the IOSQE_FIXED_FILE flag when doing submission
            // queue polling
            // IOSQE_IO_LINK to link to the next operation, which should be a
            // write()
            io_uring_sqe_set_flags(read_sqe, IOSQE_FIXED_FILE | IOSQE_IO_LINK);

            io_uring_prep_writev(write_sqe, 1, iovecs, num_blocks, -1);
            write_sqe->flags |=
                IOSQE_FIXED_FILE; // MUST set this flag when doing submission
            io_uring_sqe_set_data(write_sqe, NULL);

            file_offset += bytes_to_read;
            inflight_count += 2;
        }

        // Submit the entries in the submission queue
        if (inflight_count)
            io_uring_submit(ring);

        // print_sq_poll_kernel_thread_status();
        // consume the completion queue entries
        while (inflight_count) {
            struct io_uring_cqe *cqe;
            ret = io_uring_peek_cqe(ring, &cqe);
            if (ret == -EAGAIN) {
                // No completions are ready.
                // Break and try to submit more.
                break;
            }

            if (ret < 0) {
                fprintf(stderr, "io_uring_peek_cqe: %s\n", strerror(-ret));
                break;
            }

            if (cqe->res < 0) {
                fprintf(stderr, "CQE operation failed: %s\n",
                        strerror(-cqe->res));
            }

            io_uring_cqe_seen(
                ring, cqe); // MUST call this after calling io_uring_peek_cqe
            inflight_count--;
        }
    }

    close(file_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
        return 1;
    }

    struct io_uring ring;
    struct io_uring_params params;
    memset(&params, 0, sizeof(params));
    params.flags |= IORING_SETUP_SQPOLL;
    params.sq_thread_idle =
        3000; // if no new sqe found within 2s while polling, exit polling. Next
              // call to `io_uring_submit` will call the syscall
              // `io_uring_enter`.
    io_uring_queue_init_params(
        QUEUE_DEPTH, &ring,
        &params); // kernel creates its own dedicated kernel thread
                  // that polls for submission queue entries

    char *buffer_pool = malloc(UIO_MAXIOV * BLOCK_SZ);
    if (!buffer_pool) {
        perror("malloc() error for buffer_pool");
        return 1;
    }
    for (int i = 0; i < UIO_MAXIOV; i++) {
        iovecs[i].iov_base = buffer_pool + i * BLOCK_SZ;
        if (!iovecs[i].iov_base) {
            perror("malloc() error for iovecs");
            return 1;
        }
        iovecs[i].iov_len = BLOCK_SZ;
    }

    // Register the buffers with the kernel. This allows the kernel to keep
    // long-term references of internal data structures This should reduce I/O
    // overhead.
    if (io_uring_register_buffers(&ring, iovecs, QUEUE_DEPTH) < 0) {
        perror("io_uring_register_buffers");
        return 11;
    }

    // print_sq_poll_kernel_thread_status();

    for (int i = 1; i < argc; i++) {
        int ret = read_and_print_file(argv[i], &ring);
        if (ret) {
            fprintf(stderr, "Error reading file: %s\n", argv[i]);
            return 1;
        }
    }

    free(buffer_pool);

    /* MUST call the clean-up function. */
    io_uring_queue_exit(&ring);
    return 0;
}
