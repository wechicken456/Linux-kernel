// Simple test to verify io_uring completion ordering
#include <stdio.h>
#include <liburing.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define QUEUE_DEPTH 8

int main() {
    struct io_uring ring;
    io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    
    int fd = open("/tmp/test_ordering.txt", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    char buffers[QUEUE_DEPTH][100];
    
    // Submit multiple read operations
    for (int i = 0; i < QUEUE_DEPTH; i++) {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
        io_uring_prep_read(sqe, fd, buffers[i], 10, i * 10);
        io_uring_sqe_set_data(sqe, (void*)(long)i);
    }
    
    io_uring_submit(&ring);
    
    // Check completion order
    printf("Completion order:\n");
    for (int i = 0; i < QUEUE_DEPTH; i++) {
        struct io_uring_cqe *cqe;
        io_uring_wait_cqe(&ring, &cqe);
        int idx = (int)(long)io_uring_cqe_get_data(cqe);
        printf("  Operation %d completed (submitted as #%d)\n", idx, i);
        io_uring_cqe_seen(&ring, cqe);
    }
    
    close(fd);
    io_uring_queue_exit(&ring);
    return 0;
}
